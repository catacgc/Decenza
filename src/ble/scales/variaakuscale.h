#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>
#include <QTimer>

class VariaAkuScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit VariaAkuScale(QObject* parent = nullptr);
    ~VariaAkuScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "varia_aku"; }

public slots:
    void tare() override;

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);
    void onWatchdogTimeout();
    void onTickleTimeout();

private:
    void sendCommand(const QByteArray& cmd);
    void enableNotifications();
    void startWatchdog();
    void tickleWatchdog();
    void stopWatchdog();

    QString m_name = "Varia Aku";
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;

    // Watchdog to re-enable notifications if they stop arriving
    QTimer* m_watchdogTimer = nullptr;
    QTimer* m_tickleTimer = nullptr;
    int m_watchdogRetries = 0;
    bool m_updatesReceived = false;
    static constexpr int WATCHDOG_TIMEOUT_MS = 1000;      // Retry interval
    static constexpr int TICKLE_TIMEOUT_MS = 2000;        // No-update timeout
    static constexpr int MAX_WATCHDOG_RETRIES = 10;
};
