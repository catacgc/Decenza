#include "felicitascale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

FelicitaScale::FelicitaScale(QObject* parent)
    : ScaleDevice(parent)
{
}

FelicitaScale::~FelicitaScale() {
    disconnectFromScale();
}

void FelicitaScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &FelicitaScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &FelicitaScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &FelicitaScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &FelicitaScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void FelicitaScale::onControllerConnected() {
    m_controller->discoverServices();
}

void FelicitaScale::onControllerDisconnected() {
    setConnected(false);
}

void FelicitaScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Felicita scale connection error");
    setConnected(false);
}

void FelicitaScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::Felicita::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &FelicitaScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &FelicitaScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void FelicitaScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_characteristic = m_service->characteristic(Scale::Felicita::CHARACTERISTIC);

        // Subscribe to notifications
        if (m_characteristic.isValid()) {
            QLowEnergyDescriptor notification = m_characteristic.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
            }
        }

        setConnected(true);
    }
}

void FelicitaScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::Felicita::CHARACTERISTIC) {
        parseResponse(value);
    }
}

void FelicitaScale::parseResponse(const QByteArray& data) {
    // Felicita format: header1 header2 sign weight[6] ... battery
    if (data.size() < 9) return;

    const uint8_t* d = reinterpret_cast<const uint8_t*>(data.constData());

    // Check headers
    if (d[0] != 0x01 || d[1] != 0x02) return;

    // Sign is at byte 2 ('+' or '-')
    char sign = static_cast<char>(d[2]);

    // Weight is 6 ASCII digits starting at byte 3
    QByteArray weightStr = data.mid(3, 6);
    bool ok;
    int weightInt = weightStr.toInt(&ok);
    if (!ok) return;

    double weight = weightInt / 100.0;  // Weight in grams with 2 decimal places
    if (sign == '-') {
        weight = -weight;
    }

    setWeight(weight);

    // Battery level is at byte 15 if available
    if (data.size() >= 16) {
        uint8_t battery = d[15];
        // Battery formula from de1app: ((battery - 129) / 29.0) * 100
        int battLevel = static_cast<int>(((battery - 129) / 29.0) * 100);
        battLevel = qBound(0, battLevel, 100);
        setBatteryLevel(battLevel);
    }
}

void FelicitaScale::sendCommand(uint8_t cmd) {
    if (!m_service || !m_characteristic.isValid()) return;

    QByteArray packet;
    packet.append(static_cast<char>(cmd));
    m_service->writeCharacteristic(m_characteristic, packet);
}

void FelicitaScale::tare() {
    sendCommand(0x54);  // 'T'
}

void FelicitaScale::startTimer() {
    sendCommand(0x52);  // 'R'
}

void FelicitaScale::stopTimer() {
    sendCommand(0x53);  // 'S'
}

void FelicitaScale::resetTimer() {
    sendCommand(0x43);  // 'C'
}
