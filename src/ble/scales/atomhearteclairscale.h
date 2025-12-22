#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

class AtomheartEclairScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit AtomheartEclairScale(QObject* parent = nullptr);
    ~AtomheartEclairScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "atomheart_eclair"; }

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
    bool validateXor(const QByteArray& data);

    QString m_name = "Atomheart Eclair";
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;
};
