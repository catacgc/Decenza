#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

class SmartChefScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit SmartChefScale(QObject* parent = nullptr);
    ~SmartChefScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "smartchef"; }

public slots:
    void tare() override;  // SmartChef doesn't support software tare

private slots:
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray& value);

private:
    QString m_name = "SmartChef";
    QLowEnergyCharacteristic m_statusChar;
};
