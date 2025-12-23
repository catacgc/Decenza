#include "eurekaprecisascale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

EurekaPrecisaScale::EurekaPrecisaScale(QObject* parent)
    : ScaleDevice(parent)
{
}

EurekaPrecisaScale::~EurekaPrecisaScale() {
    disconnectFromScale();
}

void EurekaPrecisaScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &EurekaPrecisaScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &EurekaPrecisaScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &EurekaPrecisaScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &EurekaPrecisaScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void EurekaPrecisaScale::onControllerConnected() {
    m_controller->discoverServices();
}

void EurekaPrecisaScale::onControllerDisconnected() {
    setConnected(false);
}

void EurekaPrecisaScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Eureka Precisa scale connection error");
    setConnected(false);
}

void EurekaPrecisaScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::Generic::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &EurekaPrecisaScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &EurekaPrecisaScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void EurekaPrecisaScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::Generic::STATUS);
        m_cmdChar = m_service->characteristic(Scale::Generic::CMD);

        // Subscribe to status notifications
        if (m_statusChar.isValid()) {
            QLowEnergyDescriptor notification = m_statusChar.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
            }
        }

        setConnected(true);

        // Set unit to grams
        setUnitToGrams();
    }
}

void EurekaPrecisaScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::Generic::STATUS) {
        // Eureka Precisa format: AA 09 41 timer_running timer sign weight(2 bytes)
        // Header check: h1=0xAA (170), h2=0x09, h3=0x41 (65)
        if (value.size() >= 9) {
            const uint8_t* d = reinterpret_cast<const uint8_t*>(value.constData());

            // Check header
            if (d[0] != 0xAA || d[1] != 0x09 || d[2] != 0x41) {
                return;
            }

            // Weight is in bytes 6-7 as unsigned short (tenths of gram)
            uint16_t weightRaw = (d[6] << 8) | d[7];
            double weight = weightRaw / 10.0;

            // Sign is in byte 5 (1 = negative)
            if (d[5] == 1) {
                weight = -weight;
            }

            setWeight(weight);
        }
    }
}

void EurekaPrecisaScale::sendCommand(const QByteArray& cmd) {
    if (!m_service || !m_cmdChar.isValid()) return;
    m_service->writeCharacteristic(m_cmdChar, cmd);
}

void EurekaPrecisaScale::tare() {
    sendCommand(QByteArray::fromHex("AA023131"));
}

void EurekaPrecisaScale::startTimer() {
    sendCommand(QByteArray::fromHex("AA023333"));
}

void EurekaPrecisaScale::stopTimer() {
    sendCommand(QByteArray::fromHex("AA023434"));
}

void EurekaPrecisaScale::resetTimer() {
    sendCommand(QByteArray::fromHex("AA023535"));
}

void EurekaPrecisaScale::setUnitToGrams() {
    sendCommand(QByteArray::fromHex("AA033600"));
}

void EurekaPrecisaScale::turnOff() {
    sendCommand(QByteArray::fromHex("AA023232"));
}

void EurekaPrecisaScale::beepTwice() {
    sendCommand(QByteArray::fromHex("AA023737"));
}
