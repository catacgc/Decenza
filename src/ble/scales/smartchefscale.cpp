#include "smartchefscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

SmartChefScale::SmartChefScale(QObject* parent)
    : ScaleDevice(parent)
{
}

SmartChefScale::~SmartChefScale() {
    disconnect();
}

void SmartChefScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnect();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &SmartChefScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &SmartChefScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &SmartChefScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &SmartChefScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void SmartChefScale::onControllerConnected() {
    m_controller->discoverServices();
}

void SmartChefScale::onControllerDisconnected() {
    setConnected(false);
}

void SmartChefScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("SmartChef scale connection error");
    setConnected(false);
}

void SmartChefScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::Generic::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &SmartChefScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &SmartChefScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void SmartChefScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::Generic::STATUS);

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

void SmartChefScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::Generic::STATUS) {
        // SmartChef format: weight in bytes 5-6 as unsigned short (tenths of gram)
        // Sign determined by byte 3
        if (value.size() >= 7) {
            const uint8_t* d = reinterpret_cast<const uint8_t*>(value.constData());

            int16_t weightRaw = static_cast<int16_t>((d[5] << 8) | d[6]);
            double weight = weightRaw / 10.0;

            // If byte 3 > 10, weight is negative
            if (d[3] > 10) {
                weight = -weight;
            }

            setWeight(weight);
        }
    }
}

void SmartChefScale::tare() {
    // SmartChef doesn't support software-based taring
    // User must press the tare button on the scale
    qDebug() << "SmartChef scale: software tare not supported, press tare button on scale";
}
