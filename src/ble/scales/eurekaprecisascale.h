#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

class EurekaPrecisaScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit EurekaPrecisaScale(QObject* parent = nullptr);
    ~EurekaPrecisaScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "eureka_precisa"; }

public slots:
    void tare() override;
    void startTimer() override;
    void stopTimer() override;
    void resetTimer() override;

    void setUnitToGrams();
    void turnOff();
    void beepTwice();

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);

private:
    void sendCommand(const QByteArray& cmd);

    QString m_name = "Eureka Precisa";
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;
};
