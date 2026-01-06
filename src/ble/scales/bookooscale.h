#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

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

private:
    void sendCommand(const QByteArray& cmd);

    QString m_name = "Bookoo";
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;
};
