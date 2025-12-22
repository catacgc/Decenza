#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

class FelicitaScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit FelicitaScale(QObject* parent = nullptr);
    ~FelicitaScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "felicita"; }

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
    void parseResponse(const QByteArray& data);
    void sendCommand(uint8_t cmd);

    QString m_name = "Felicita";
    QLowEnergyCharacteristic m_characteristic;
};
