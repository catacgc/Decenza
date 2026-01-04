#include "bookooscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

BookooScale::BookooScale(QObject* parent)
    : ScaleDevice(parent)
{
    m_watchdogTimer.setSingleShot(true);
    connect(&m_watchdogTimer, &QTimer::timeout, this, &BookooScale::onWatchdogTimeout);
}

BookooScale::~BookooScale() {
    stopWatchdog();
    disconnectFromScale();
}

void BookooScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
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
    stopWatchdog();
    m_receivedData = false;
    setConnected(false);
}

void BookooScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    stopWatchdog();
    m_receivedData = false;
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
            connect(m_service, &QLowEnergyService::descriptorWritten,
                    this, &BookooScale::onDescriptorWritten);
            m_service->discoverDetails();
        }
    }
}

void BookooScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::Bookoo::STATUS);
        m_cmdChar = m_service->characteristic(Scale::Bookoo::CMD);

        // Reset watchdog state for new connection
        m_notificationRetries = 0;
        m_receivedData = false;

        // Delay notification subscription by 200ms (matches de1app behavior)
        // This gives the BLE stack time to stabilize after service discovery
        QTimer::singleShot(INITIAL_DELAY_MS, this, &BookooScale::enableNotifications);
    }
}

void BookooScale::enableNotifications() {
    if (!m_service || !m_statusChar.isValid()) {
        qWarning() << "Bookoo: Cannot enable notifications - service or characteristic invalid";
        return;
    }

    qDebug() << "Bookoo: Enabling notifications (attempt" << (m_notificationRetries + 1) << ")";

    // Subscribe to status notifications
    QLowEnergyDescriptor notification = m_statusChar.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (notification.isValid()) {
        m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
    }

    // Start watchdog to retry if no data received
    startWatchdog();
}

void BookooScale::onDescriptorWritten(const QLowEnergyDescriptor& descriptor, const QByteArray& value) {
    Q_UNUSED(descriptor)
    Q_UNUSED(value)
    qDebug() << "Bookoo: Notification descriptor written successfully";
    // Don't set connected here - wait for actual weight data
}

void BookooScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::Bookoo::STATUS) {
        // First data received - we're truly connected now
        if (!m_receivedData) {
            m_receivedData = true;
            stopWatchdog();
            setConnected(true);
            qDebug() << "Bookoo: First weight data received, connection confirmed";
        }

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

void BookooScale::startWatchdog() {
    m_watchdogTimer.start(WATCHDOG_INTERVAL_MS);
}

void BookooScale::stopWatchdog() {
    m_watchdogTimer.stop();
    m_notificationRetries = 0;
}

void BookooScale::onWatchdogTimeout() {
    // If we've received data, the scale is working - no need to retry
    if (m_receivedData) {
        return;
    }

    m_notificationRetries++;

    if (m_notificationRetries >= MAX_NOTIFICATION_RETRIES) {
        qWarning() << "Bookoo: Failed to receive weight data after"
                   << MAX_NOTIFICATION_RETRIES << "attempts, giving up";
        emit errorOccurred("Bookoo scale not responding - no weight data received");
        // Don't disconnect - the BLE connection might still be valid,
        // but we couldn't get notifications working
        return;
    }

    qDebug() << "Bookoo: No weight data received, retrying notification subscription"
             << "(" << m_notificationRetries << "/" << MAX_NOTIFICATION_RETRIES << ")";

    // Retry enabling notifications (matches de1app watchdog_first behavior)
    enableNotifications();
}
