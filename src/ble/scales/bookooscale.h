#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>
#include <QTimer>

class BookooScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit BookooScale(QObject* parent = nullptr);
    ~BookooScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "bookoo"; }

public slots:
    void tare() override;
    void startTimer() override;
    void stopTimer() override;
    void resetTimer() override;

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);
    void onDescriptorWritten(const QLowEnergyDescriptor& descriptor, const QByteArray& value);
    void enableNotifications();
    void onWatchdogTimeout();

private:
    void sendCommand(const QByteArray& cmd);
    void startWatchdog();
    void stopWatchdog();

    QString m_name = "Bookoo";
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;

    // Watchdog for notification retry (matches de1app behavior)
    QTimer m_watchdogTimer;
    int m_notificationRetries = 0;
    bool m_receivedData = false;
    static const int MAX_NOTIFICATION_RETRIES = 10;
    static const int WATCHDOG_INTERVAL_MS = 1000;
    static const int INITIAL_DELAY_MS = 200;
};
