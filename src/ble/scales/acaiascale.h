#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>
#include <QTimer>
#include <QByteArray>

class AcaiaScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit AcaiaScale(bool isPyxis = false, QObject* parent = nullptr);
    ~AcaiaScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return m_isPyxis ? "acaiapyxis" : "acaia"; }

public slots:
    void tare() override;
    void startTimer() override {}  // Acaia scales don't support remote timer control
    void stopTimer() override {}
    void resetTimer() override {}

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);
    void sendHeartbeat();
    void sendIdent();
    void sendConfig();

private:
    void parseResponse(const QByteArray& data);
    void decodeWeight(const QByteArray& payload, int payloadOffset);
    QByteArray encodePacket(uint8_t msgType, const QByteArray& payload);
    void sendCommand(const QByteArray& command);

    QString m_name = "Acaia";
    bool m_isPyxis;
    bool m_receivingNotifications = false;
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;
    QTimer* m_heartbeatTimer = nullptr;

    // Message parsing state
    QByteArray m_buffer;
    static constexpr int ACAIA_METADATA_LEN = 5;
};
