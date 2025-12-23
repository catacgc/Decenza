#pragma once

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QList>
#include <QVariant>
#include <QPermissions>
#include <QTimer>

class ScaleDevice;

class BLEManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QVariantList discoveredDevices READ discoveredDevices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList discoveredScales READ discoveredScales NOTIFY scalesChanged)
    Q_PROPERTY(bool autoScanForScale READ autoScanForScale WRITE setAutoScanForScale NOTIFY autoScanForScaleChanged)

public:
    explicit BLEManager(QObject* parent = nullptr);
    ~BLEManager();

    bool isScanning() const;
    QVariantList discoveredDevices() const;
    QVariantList discoveredScales() const;
    bool autoScanForScale() const { return m_autoScanForScale; }

    Q_INVOKABLE QBluetoothDeviceInfo getScaleDeviceInfo(const QString& address) const;
    Q_INVOKABLE QString getScaleType(const QString& address) const;

    void setScaleDevice(ScaleDevice* scale);
    void setAutoScanForScale(bool enabled);

    // Direct connect to wake sleeping scales
    void setSavedScaleAddress(const QString& address, const QString& type);
    void tryDirectConnectToScale();

public slots:
    void startScan();
    void stopScan();
    void clearDevices();

signals:
    void scanningChanged();
    void devicesChanged();
    void scalesChanged();
    void autoScanForScaleChanged();
    void de1Discovered(const QBluetoothDeviceInfo& device);
    void scaleDiscovered(const QBluetoothDeviceInfo& device, const QString& type);
    void errorOccurred(const QString& error);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo& device);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onScaleConnectedChanged();
    void restartScanForScale();

private:
    bool isDE1Device(const QBluetoothDeviceInfo& device) const;
    QString getScaleType(const QBluetoothDeviceInfo& device) const;
    void requestBluetoothPermission();
    void doStartScan();
    bool shouldContinueScanning() const;

    QBluetoothDeviceDiscoveryAgent* m_discoveryAgent = nullptr;
    QList<QBluetoothDeviceInfo> m_de1Devices;
    QList<QPair<QBluetoothDeviceInfo, QString>> m_scales;  // device, type
    bool m_scanning = false;
    bool m_permissionRequested = false;
    bool m_autoScanForScale = true;  // Auto-scan until scale found
    ScaleDevice* m_scaleDevice = nullptr;
    QTimer* m_rescanTimer = nullptr;

    // Saved scale for direct wake connection
    QString m_savedScaleAddress;
    QString m_savedScaleType;
};
