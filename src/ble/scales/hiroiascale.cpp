#include "hiroiascale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

HiroiaScale::HiroiaScale(QObject* parent)
    : ScaleDevice(parent)
{
}

HiroiaScale::~HiroiaScale() {
    disconnectFromScale();
}

void HiroiaScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &HiroiaScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &HiroiaScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &HiroiaScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &HiroiaScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void HiroiaScale::onControllerConnected() {
    m_controller->discoverServices();
}

void HiroiaScale::onControllerDisconnected() {
    setConnected(false);
}

void HiroiaScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Hiroia Jimmy scale connection error");
    setConnected(false);
}

void HiroiaScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::HiroiaJimmy::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &HiroiaScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &HiroiaScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void HiroiaScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_cmdChar = m_service->characteristic(Scale::HiroiaJimmy::CMD);
        m_statusChar = m_service->characteristic(Scale::HiroiaJimmy::STATUS);

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

void HiroiaScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::HiroiaJimmy::STATUS) {
        // Hiroia format: 4 bytes header, then 4 bytes weight (unsigned, tenths of gram)
        if (value.size() >= 7) {
            // Append a zero byte to make it 8 bytes for easy parsing
            QByteArray padded = value;
            padded.append(static_cast<char>(0));

            const uint8_t* d = reinterpret_cast<const uint8_t*>(padded.constData());

            // Weight is in bytes 4-7 as unsigned 32-bit little-endian
            uint32_t weightRaw = d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24);

            // Handle negative values (if >= 8388608, it's negative)
            double weight;
            if (weightRaw >= 8388608) {
                weight = static_cast<double>(0xFFFFFF - weightRaw) * -1.0;
            } else {
                weight = static_cast<double>(weightRaw);
            }

            weight /= 10.0;  // Convert to grams
            setWeight(weight);
        }
    }
}

void HiroiaScale::tare() {
    if (!m_service || !m_cmdChar.isValid()) return;

    QByteArray packet = QByteArray::fromHex("0700");
    m_service->writeCharacteristic(m_cmdChar, packet);
}
