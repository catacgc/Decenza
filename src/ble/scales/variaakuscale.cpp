#include "variaakuscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>

VariaAkuScale::VariaAkuScale(QObject* parent)
    : ScaleDevice(parent)
{
    // Create watchdog timer (fires if no updates received after enable)
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setSingleShot(true);
    connect(m_watchdogTimer, &QTimer::timeout, this, &VariaAkuScale::onWatchdogTimeout);

    // Create tickle timer (fires if updates stop arriving)
    m_tickleTimer = new QTimer(this);
    m_tickleTimer->setSingleShot(true);
    connect(m_tickleTimer, &QTimer::timeout, this, &VariaAkuScale::onTickleTimeout);
}

VariaAkuScale::~VariaAkuScale() {
    stopWatchdog();
    disconnectFromScale();
}

void VariaAkuScale::connectToDevice(const QBluetoothDeviceInfo& device) {
    if (m_controller) {
        disconnectFromScale();
    }

    m_name = device.name();
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &VariaAkuScale::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &VariaAkuScale::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &VariaAkuScale::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &VariaAkuScale::onServiceDiscovered);

    m_controller->connectToDevice();
}

void VariaAkuScale::onControllerConnected() {
    m_controller->discoverServices();
}

void VariaAkuScale::onControllerDisconnected() {
    stopWatchdog();
    setConnected(false);
}

void VariaAkuScale::onControllerError(QLowEnergyController::Error error) {
    Q_UNUSED(error)
    emit errorOccurred("Varia Aku scale connection error");
    setConnected(false);
}

void VariaAkuScale::onServiceDiscovered(const QBluetoothUuid& uuid) {
    if (uuid == Scale::VariaAku::SERVICE) {
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &VariaAkuScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &VariaAkuScale::onCharacteristicChanged);
            m_service->discoverDetails();
        }
    }
}

void VariaAkuScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::VariaAku::STATUS);
        m_cmdChar = m_service->characteristic(Scale::VariaAku::CMD);

        // Delay notification enable by 200ms (matching de1app pattern)
        // The Varia Aku scale needs time to stabilize after service discovery
        QTimer::singleShot(200, this, [this]() {
            // Check if still connected (scale may have disconnected during delay)
            if (!m_service || !m_controller) {
                return;
            }

            // Enable notifications and start watchdog
            enableNotifications();
            startWatchdog();

            setConnected(true);
        });
    }
}

void VariaAkuScale::enableNotifications() {
    if (!m_service || !m_statusChar.isValid()) {
        return;
    }

    qDebug() << "Varia Aku: enabling weight notifications";

    QLowEnergyDescriptor notification = m_statusChar.descriptor(
        QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (notification.isValid()) {
        m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
    }
}

void VariaAkuScale::startWatchdog() {
    m_watchdogRetries = 0;
    m_updatesReceived = false;
    m_watchdogTimer->start(WATCHDOG_TIMEOUT_MS);
    qDebug() << "Varia Aku: watchdog started";
}

void VariaAkuScale::tickleWatchdog() {
    // First update received - watchdog succeeded
    if (!m_updatesReceived) {
        m_updatesReceived = true;
        m_watchdogTimer->stop();
        qDebug() << "Varia Aku: first weight update received, watchdog stopped";
    }

    // Reset the tickle timer - if no updates for TICKLE_TIMEOUT_MS, log warning
    m_tickleTimer->start(TICKLE_TIMEOUT_MS);
}

void VariaAkuScale::stopWatchdog() {
    m_watchdogTimer->stop();
    m_tickleTimer->stop();
    m_updatesReceived = false;
    m_watchdogRetries = 0;
}

void VariaAkuScale::onWatchdogTimeout() {
    if (m_updatesReceived) {
        return;  // Updates started arriving, no need to retry
    }

    m_watchdogRetries++;

    if (m_watchdogRetries >= MAX_WATCHDOG_RETRIES) {
        qWarning() << "Varia Aku: no weight updates after" << MAX_WATCHDOG_RETRIES
                   << "retries, giving up";
        emit errorOccurred("Varia Aku scale not sending weight updates");
        return;
    }

    qDebug() << "Varia Aku: no weight updates, retry" << m_watchdogRetries
             << "of" << MAX_WATCHDOG_RETRIES;

    // Re-enable notifications
    enableNotifications();

    // Schedule next retry
    m_watchdogTimer->start(WATCHDOG_TIMEOUT_MS);
}

void VariaAkuScale::onTickleTimeout() {
    // No updates received for TICKLE_TIMEOUT_MS
    qWarning() << "Varia Aku: no weight updates for" << TICKLE_TIMEOUT_MS << "ms";

    // Try re-enabling notifications
    m_updatesReceived = false;
    m_watchdogRetries = 0;
    enableNotifications();
    m_watchdogTimer->start(WATCHDOG_TIMEOUT_MS);
}

void VariaAkuScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::VariaAku::STATUS) {
        // Varia Aku format: header command length payload xor
        // Weight notification: command 0x01, length 0x03, payload w1 w2 w3 xor
        if (value.size() >= 4) {
            const uint8_t* d = reinterpret_cast<const uint8_t*>(value.constData());

            uint8_t command = d[1];
            uint8_t length = d[2];

            // Weight notification
            if (command == 0x01 && length == 0x03 && value.size() >= 7) {
                // Tickle watchdog on every weight update
                tickleWatchdog();

                uint8_t w1 = d[3];
                uint8_t w2 = d[4];
                uint8_t w3 = d[5];

                // Sign is in highest nibble of w1 (0x10 means negative)
                bool isNegative = (w1 & 0x10) != 0;

                // Weight is 3 bytes big-endian in hundredths of gram
                // Strip sign nibble from w1
                uint32_t weightRaw = ((w1 & 0x0F) << 16) | (w2 << 8) | w3;
                double weight = weightRaw / 100.0;

                if (isNegative) {
                    weight = -weight;
                }

                setWeight(weight);
            }
            // Battery notification
            else if (command == 0x85 && length == 0x01 && value.size() >= 5) {
                uint8_t battery = d[3];
                setBatteryLevel(battery);
            }
        }
    }
}

void VariaAkuScale::sendCommand(const QByteArray& cmd) {
    if (!m_service || !m_cmdChar.isValid()) return;
    m_service->writeCharacteristic(m_cmdChar, cmd);
}

void VariaAkuScale::tare() {
    sendCommand(QByteArray::fromHex("FA82010182"));
}
