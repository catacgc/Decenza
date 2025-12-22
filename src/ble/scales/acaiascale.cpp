#include "acaiascale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>
#include <cmath>

AcaiaScale::AcaiaScale(bool isPyxis, QObject* parent)
    : ScaleDevice(parent)
    , m_isPyxis(isPyxis)
{
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &AcaiaScale::sendHeartbeat);
}

AcaiaScale::~AcaiaScale() {
    m_heartbeatTimer->stop();
    disconnect();
}

void AcaiaScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnect();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &AcaiaScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &AcaiaScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &AcaiaScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &AcaiaScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void AcaiaScale::onControllerConnected() {
    m_controller->discoverServices();
}

void AcaiaScale::onControllerDisconnected() {
    m_heartbeatTimer->stop();
    setConnected(false);
}

void AcaiaScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    m_heartbeatTimer->stop();
    emit errorOccurred("Acaia scale connection error");
    setConnected(false);
}

void AcaiaScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    // Check for Acaia service (Pyxis or IPS)
    QBluetoothUuid targetService = m_isPyxis ? Scale::Acaia::SERVICE : Scale::AcaiaIPS::SERVICE;

    if (uuid == targetService) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &AcaiaScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &AcaiaScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void AcaiaScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        if (m_isPyxis) {
            m_statusChar = m_service->characteristic(Scale::Acaia::STATUS);
            m_cmdChar = m_service->characteristic(Scale::Acaia::CMD);
        } else {
            m_statusChar = m_service->characteristic(Scale::AcaiaIPS::CHARACTERISTIC);
            m_cmdChar = m_statusChar;  // Same characteristic for IPS
        }

        // Subscribe to notifications
        if (m_statusChar.isValid()) {
            QLowEnergyDescriptor notification = m_statusChar.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
            }
        }

        setConnected(true);

        // Start the Acaia handshake sequence
        m_receivingNotifications = false;
        QTimer::singleShot(100, this, &AcaiaScale::sendIdent);
    }
}

void AcaiaScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == m_statusChar.uuid()) {
        parseResponse(value);
    }
}

QByteArray AcaiaScale::encodePacket(uint8_t msgType, const QByteArray& payload) {
    QByteArray packet;
    packet.append(static_cast<char>(0xEF));  // Header 1
    packet.append(static_cast<char>(0xDD));  // Header 2
    packet.append(static_cast<char>(msgType));
    packet.append(payload);
    return packet;
}

void AcaiaScale::sendIdent() {
    // Ident message: type 0x0B with "01234567890123" + checksum
    QByteArray payload = QByteArray::fromHex("3031323334353637383930313233349A6D");
    QByteArray packet = encodePacket(0x0B, payload);
    sendCommand(packet);

    if (!m_receivingNotifications) {
        // Retry ident and then send config
        QTimer::singleShot(400, this, &AcaiaScale::sendIdent);
        QTimer::singleShot(1000, this, &AcaiaScale::sendConfig);
    } else {
        QTimer::singleShot(400, this, &AcaiaScale::sendConfig);
        QTimer::singleShot(1500, this, &AcaiaScale::sendHeartbeat);
    }
}

void AcaiaScale::sendConfig() {
    // Config message: type 0x0C with notification settings
    QByteArray payload = QByteArray::fromHex("0900010102020103041106");
    QByteArray packet = encodePacket(0x0C, payload);
    sendCommand(packet);
}

void AcaiaScale::sendHeartbeat() {
    // Heartbeat message: type 0x00 with status bytes
    QByteArray payload = QByteArray::fromHex("02000200");
    QByteArray packet = encodePacket(0x00, payload);
    sendCommand(packet);

    // Schedule next heartbeat
    m_heartbeatTimer->start(2000);
}

void AcaiaScale::sendCommand(const QByteArray& command) {
    if (!m_service || !m_cmdChar.isValid()) return;

    // Use write without response for Pyxis
    if (m_isPyxis) {
        m_service->writeCharacteristic(m_cmdChar, command, QLowEnergyService::WriteWithoutResponse);
    } else {
        m_service->writeCharacteristic(m_cmdChar, command);
    }
}

void AcaiaScale::parseResponse(const QByteArray& data) {
    // Append to buffer
    m_buffer.append(data);

    // Need at least 6 bytes for a valid message
    if (m_buffer.size() < 6) return;

    const uint8_t* buf = reinterpret_cast<const uint8_t*>(m_buffer.constData());

    // Find message start (0xEF 0xDD)
    int msgStart = -1;
    for (int i = 0; i < m_buffer.size() - 1; i++) {
        if (buf[i] == 0xEF && buf[i + 1] == 0xDD) {
            msgStart = i;
            break;
        }
    }

    if (msgStart < 0) {
        m_buffer.clear();
        return;
    }

    // Skip bytes before message start
    if (msgStart > 0) {
        m_buffer = m_buffer.mid(msgStart);
        buf = reinterpret_cast<const uint8_t*>(m_buffer.constData());
    }

    // Check if we have enough data for metadata
    if (m_buffer.size() < ACAIA_METADATA_LEN + 1) return;

    uint8_t msgType = buf[2];
    uint8_t length = buf[3];
    uint8_t eventType = buf[4];

    // Mark that we're receiving notifications (not just info messages)
    if (msgType != 7) {
        m_receivingNotifications = true;
    }

    // Check if we have the complete message
    int msgEnd = ACAIA_METADATA_LEN + length;
    if (m_buffer.size() < msgEnd) return;

    // Only process weight messages (msgType 0x0C, eventType 5 or 11)
    if (msgType == 0x0C && (eventType == 5 || eventType == 11)) {
        int payloadOffset = (eventType == 5) ? ACAIA_METADATA_LEN : ACAIA_METADATA_LEN + 3;
        decodeWeight(m_buffer, payloadOffset);
    }

    // Clear buffer after processing
    m_buffer.clear();
}

void AcaiaScale::decodeWeight(const QByteArray& data, int payloadOffset) {
    if (data.size() < payloadOffset + 6) return;

    const uint8_t* payload = reinterpret_cast<const uint8_t*>(data.constData()) + payloadOffset;

    // Weight is 3 bytes, little-endian
    int32_t value = ((payload[2] & 0xFF) << 16) |
                    ((payload[1] & 0xFF) << 8) |
                    (payload[0] & 0xFF);

    // Unit is in payload[4]
    uint8_t unit = payload[4] & 0xFF;
    double weight = value / std::pow(10.0, unit);

    // Sign is in payload[5]
    bool isNegative = payload[5] > 1;
    if (isNegative) {
        weight = -weight;
    }

    setWeight(weight);
}

void AcaiaScale::tare() {
    // Tare message: type 0x04 with zeros
    QByteArray payload(17, 0);
    QByteArray packet = encodePacket(0x04, payload);
    sendCommand(packet);
}
