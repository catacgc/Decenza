#include "atomhearteclairscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

AtomheartEclairScale::AtomheartEclairScale(QObject* parent)
    : ScaleDevice(parent)
{
}

AtomheartEclairScale::~AtomheartEclairScale() {
    disconnectFromScale();
}

void AtomheartEclairScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &AtomheartEclairScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &AtomheartEclairScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &AtomheartEclairScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &AtomheartEclairScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void AtomheartEclairScale::onControllerConnected() {
    m_controller->discoverServices();
}

void AtomheartEclairScale::onControllerDisconnected() {
    setConnected(false);
}

void AtomheartEclairScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Atomheart Eclair scale connection error");
    setConnected(false);
}

void AtomheartEclairScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::AtomheartEclair::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &AtomheartEclairScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &AtomheartEclairScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void AtomheartEclairScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::AtomheartEclair::STATUS);
        m_cmdChar = m_service->characteristic(Scale::AtomheartEclair::CMD);

        // Subscribe to status notifications
        if (m_statusChar.isValid()) {
            QLowEnergyDescriptor notification = m_statusChar.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
            }
        }

        setConnected(true);
    }
}

bool AtomheartEclairScale::validateXor(const QByteArray& data) {
    if (data.size() < 2) return false;

    const uint8_t* d = reinterpret_cast<const uint8_t*>(data.constData());

    // XOR all bytes except header and last (XOR) byte
    uint8_t xorResult = 0;
    for (int i = 1; i < data.size() - 1; i++) {
        xorResult ^= d[i];
    }

    return xorResult == d[data.size() - 1];
}

void AtomheartEclairScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::AtomheartEclair::STATUS) {
        // Atomheart Eclair format: 'W' (0x57) header, 4-byte weight in milligrams, 4-byte timer, XOR byte
        if (value.size() >= 9) {
            const uint8_t* d = reinterpret_cast<const uint8_t*>(value.constData());

            // Check header is 'W' (0x57)
            if (d[0] != 0x57) return;

            // Validate XOR checksum
            if (!validateXor(value)) {
                qDebug() << "Atomheart Eclair: XOR checksum failed";
                return;
            }

            // Weight is 4-byte signed int32 in milligrams (little-endian)
            int32_t weightMg = d[1] | (d[2] << 8) | (d[3] << 16) | (d[4] << 24);
            double weight = weightMg / 1000.0;  // Convert to grams

            setWeight(weight);
        }
    }
}

void AtomheartEclairScale::sendCommand(const QByteArray& cmd) {
    if (!m_service || !m_cmdChar.isValid()) return;
    m_service->writeCharacteristic(m_cmdChar, cmd);
}

void AtomheartEclairScale::tare() {
    sendCommand(QByteArray::fromHex("540101"));
}

void AtomheartEclairScale::startTimer() {
    sendCommand(QByteArray::fromHex("430101"));
}

void AtomheartEclairScale::stopTimer() {
    sendCommand(QByteArray::fromHex("430000"));
}

void AtomheartEclairScale::resetTimer() {
    // Eclair resets timer on tare
    tare();
}
