#pragma once

#include "../scaledevice.h"
#include <QLowEnergyCharacteristic>

class HiroiaScale : public ScaleDevice {
    Q_OBJECT

public:
    explicit HiroiaScale(QObject* parent = nullptr);
    ~HiroiaScale() override;

    void connectToDevice(const QBluetoothDeviceInfo& device) override;
    QString name() const override { return m_name; }
    QString type() const override { return "hiroiajimmy"; }

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
    QString m_name = "Hiroia Jimmy";
    QLowEnergyCharacteristic m_cmdChar;
    QLowEnergyCharacteristic m_statusChar;
};
