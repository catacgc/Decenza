#include "bookooscale.h"
#include "../protocol/de1characteristics.h"
#include <QDebug>
#include <QTimer>

// Helper macro that logs to both qDebug and emits signal for UI/file logging
#define BOOKOO_LOG(msg) do { \
    QString _msg = msg; \
    qDebug().noquote() << _msg; \
    emit logMessage(_msg); \
} while(0)

BookooScale::BookooScale(QObject* parent)
    : ScaleDevice(parent)
{
}

BookooScale::~BookooScale() {
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
    BOOKOO_LOG("Bookoo: Controller connected, starting service discovery");
    connect(m_controller, &QLowEnergyController::discoveryFinished,
            this, [this]() {
        BOOKOO_LOG(QString("Bookoo: Service discovery finished, service found: %1").arg(m_service != nullptr));
        if (!m_service) {
            BOOKOO_LOG(QString("Bookoo: Service %1 not found!").arg(Scale::Bookoo::SERVICE.toString()));
            emit errorOccurred("Bookoo service not found");
        }
    });
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
    BOOKOO_LOG(QString("Bookoo: Service discovered: %1").arg(uuid.toString()));
    if (uuid == Scale::Bookoo::SERVICE) {
        BOOKOO_LOG("Bookoo: Found Bookoo service, creating service object");
        m_service = m_controller->createServiceObject(uuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                    this, &BookooScale::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                    this, &BookooScale::onCharacteristicChanged);
            connect(m_service, &QLowEnergyService::errorOccurred,
                    this, [this](QLowEnergyService::ServiceError error) {
                // Log but don't fail - Bookoo rejects CCCD writes but may still work
                BOOKOO_LOG(QString("Bookoo: Service error: %1 (may be expected)").arg(static_cast<int>(error)));
            });
            m_service->discoverDetails();
        }
    }
}

void BookooScale::onServiceStateChanged(QLowEnergyService::ServiceState state) {
    BOOKOO_LOG(QString("Bookoo: Service state changed to: %1").arg(static_cast<int>(state)));
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        m_statusChar = m_service->characteristic(Scale::Bookoo::STATUS);
        m_cmdChar = m_service->characteristic(Scale::Bookoo::CMD);

        BOOKOO_LOG(QString("Bookoo: STATUS char valid: %1, properties: %2")
                   .arg(m_statusChar.isValid())
                   .arg(static_cast<int>(m_statusChar.properties())));
        BOOKOO_LOG(QString("Bookoo: CMD char valid: %1").arg(m_cmdChar.isValid()));

        if (!m_statusChar.isValid()) {
            BOOKOO_LOG("Bookoo: STATUS characteristic not found!");
            emit errorOccurred("Bookoo STATUS characteristic not found");
            return;
        }

        // de1app waits 200ms after connection before enabling notifications:
        //   after 200 bookoo_enable_weight_notifications
        // Let's do the same
        QTimer::singleShot(200, this, [this]() {
            if (!m_service || !m_statusChar.isValid()) return;

            QLowEnergyDescriptor cccd = m_statusChar.descriptor(
                QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

            if (cccd.isValid()) {
                BOOKOO_LOG("Bookoo: Enabling notifications via CCCD");
                m_service->writeDescriptor(cccd, QByteArray::fromHex("0100"));
            } else {
                BOOKOO_LOG("Bookoo: No CCCD descriptor found");
            }

            setConnected(true);
            BOOKOO_LOG("Bookoo: Connected, waiting for weight data");
        });
    }
}

void BookooScale::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value) {
    if (c.uuid() == Scale::Bookoo::STATUS) {
        // Bookoo format: h1 h2 h3 h4 h5 h6 sign w1 w2 w3 (10 bytes)
        // de1app checks >= 9 bytes, we check >= 10 to be safe
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
