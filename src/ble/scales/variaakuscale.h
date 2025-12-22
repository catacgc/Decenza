#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

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

private:
    void sendCommand(const QByteArray& cmd);

    QString m_name = "Varia Aku";
    QLowEnergyCharacteristic m_statusChar;
    QLowEnergyCharacteristic m_cmdChar;
};
