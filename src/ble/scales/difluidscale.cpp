#include "difluidscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

DifluidScale::DifluidScale(QObject* parent)
    : ScaleDevice(parent)
{
}

DifluidScale::~DifluidScale() {
    disconnectFromScale();
}

void DifluidScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &DifluidScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &DifluidScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &DifluidScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &DifluidScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void DifluidScale::onControllerConnected() {
    m_controller->discoverServices();
}

void DifluidScale::onControllerDisconnected() {
    setConnected(false);
}

void DifluidScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Difluid scale connection error");
    setConnected(false);
}

void DifluidScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::DiFluid::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &DifluidScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &DifluidScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void DifluidScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_characteristic = m_service->characteristic(Scale::DiFluid::CHARACTERISTIC);

        // Subscribe to notifications
        if (m_characteristic.isValid()) {
            QLowEnergyDescriptor notification = m_characteristic.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
            }
        }

        setConnected(true);

        // Enable auto-notifications and set to grams
        enableNotifications();
        setToGrams();
    }
}

void DifluidScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::DiFluid::CHARACTERISTIC) {
        // Difluid format: header bytes, then hex-encoded weight
        if (value.size() >= 19) {
            // Weight is in bytes 5-8 as hex-encoded integer
            QString hexStr = value.mid(5, 8).toHex();
            bool ok;
            uint32_t weightRaw = hexStr.toUInt(&ok, 16);

            if (ok && weightRaw < 20000) {
                double weight = weightRaw / 10.0;
                setWeight(weight);
            }
        }
    }
}

void DifluidScale::sendCommand(const QByteArray& cmd) {
    if (!m_service || !m_characteristic.isValid()) return;
    m_service->writeCharacteristic(m_characteristic, cmd);
}

void DifluidScale::enableNotifications() {
    // Enable auto-notifications message
    sendCommand(QByteArray::fromHex("DFDF01000101C1"));
}

void DifluidScale::setToGrams() {
    // Set unit to grams
    sendCommand(QByteArray::fromHex("DFDF01040100C4"));
}

void DifluidScale::tare() {
    sendCommand(QByteArray::fromHex("DFDF03020101C5"));
}

void DifluidScale::startTimer() {
    sendCommand(QByteArray::fromHex("DFDF03020100C4"));
}

void DifluidScale::stopTimer() {
    sendCommand(QByteArray::fromHex("DFDF03010100C3"));
}

void DifluidScale::resetTimer() {
    sendCommand(QByteArray::fromHex("DFDF03020100C4"));
}
