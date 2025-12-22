#include "bookooscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

BookooScale::BookooScale(QObject* parent)
    : ScaleDevice(parent)
{
}

BookooScale::~BookooScale() {
    disconnect();
}

void BookooScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnect();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &BookooScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &BookooScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &BookooScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &BookooScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void BookooScale::onControllerConnected() {
    m_controller->discoverServices();
}

void BookooScale::onControllerDisconnected() {
    setConnected(false);
}

void BookooScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Bookoo scale connection error");
    setConnected(false);
}

void BookooScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::Bookoo::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &BookooScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &BookooScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void BookooScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::Bookoo::STATUS);
        m_cmdChar = m_service->characteristic(Scale::Bookoo::CMD);

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

void BookooScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::Bookoo::STATUS) {
        // Bookoo format: h1 h2 h3 h4 h5 h6 sign w1 w2 w3
        if (value.size() >= 10) {
            const uint8_t* d = reinterpret_cast<const uint8_t*>(value.constData());

            char sign = static_cast<char>(d[6]);

            // Weight is 3 bytes big-endian in hundredths of gram
            uint32_t weightRaw = (d[7] << 16) | (d[8] << 8) | d[9];
            double weight = weightRaw / 100.0;

            if (sign == '-') {
                weight = -weight;
            }

            setWeight(weight);
        }
    }
}

void BookooScale::sendCommand(const QByteArray& cmd) {
    if (!m_service || !m_cmdChar.isValid()) return;
    m_service->writeCharacteristic(m_cmdChar, cmd);
}

void BookooScale::tare() {
    sendCommand(QByteArray::fromHex("030A01000008"));
}

void BookooScale::startTimer() {
    sendCommand(QByteArray::fromHex("030A0400000A"));
}

void BookooScale::stopTimer() {
    sendCommand(QByteArray::fromHex("030A0500000D"));
}

void BookooScale::resetTimer() {
    sendCommand(QByteArray::fromHex("030A0600000C"));
}
