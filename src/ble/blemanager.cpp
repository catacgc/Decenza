#include "blemanager.h"
#include "scaledevice.h"
#include "protocol/de1characteristics.h"
#include "scales/scalefactory.h"
#include <QBluetoothUuid>
#include <QCoreApplication>
#include <QDebug>

BLEManager::BLEManager(QObject* parent)
    : QObject(parent)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(15000);  // 15 seconds per scan cycle

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BLEManager::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BLEManager::onScanFinished);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BLEManager::onScanError);

    // Timer for restarting scan when looking for scale
    m_rescanTimer = new QTimer(this);
    m_rescanTimer->setSingleShot(true);
    m_rescanTimer->setInterval(5000);  // 5 second delay between scans
    connect(m_rescanTimer, &QTimer::timeout, this, &BLEManager::restartScanForScale);
}

BLEManager::~BLEManager() {
    if (m_scanning) {
        stopScan();
    }
}

bool BLEManager::isScanning() const {
    return m_scanning;
}

QVariantList BLEManager::discoveredDevices() const {
    QVariantList result;
    for (const auto& device : m_de1Devices) {
        QVariantMap map;
        map["name"] = device.name();
        map["address"] = device.address().toString();
        result.append(map);
    }
    return result;
}

QVariantList BLEManager::discoveredScales() const {
    QVariantList result;
    for (const auto& pair : m_scales) {
        QVariantMap map;
        map["name"] = pair.first.name();
        map["address"] = pair.first.address().toString();
        map["type"] = pair.second;
        result.append(map);
    }
    return result;
}

QBluetoothDeviceInfo BLEManager::getScaleDeviceInfo(const QString& address) const {
    QBluetoothAddress addr(address);
    for (const auto& pair : m_scales) {
        if (pair.first.address() == addr) {
            return pair.first;
        }
    }
    return QBluetoothDeviceInfo();
}

QString BLEManager::getScaleType(const QString& address) const {
    QBluetoothAddress addr(address);
    for (const auto& pair : m_scales) {
        if (pair.first.address() == addr) {
            return pair.second;
        }
    }
    return QString();
}

void BLEManager::startScan() {
    if (m_scanning) return;

    // Check and request Bluetooth permission on Android
    requestBluetoothPermission();
}

void BLEManager::requestBluetoothPermission() {
#ifdef Q_OS_ANDROID
    QBluetoothPermission bluetoothPermission;
    bluetoothPermission.setCommunicationModes(QBluetoothPermission::Access);

    switch (qApp->checkPermission(bluetoothPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qDebug() << "Bluetooth permission undetermined, requesting...";
        qApp->requestPermission(bluetoothPermission, this, [this](const QPermission& permission) {
            if (permission.status() == Qt::PermissionStatus::Granted) {
                qDebug() << "Bluetooth permission granted";
                doStartScan();
            } else {
                qDebug() << "Bluetooth permission denied";
                emit errorOccurred("Bluetooth permission denied");
            }
        });
        return;
    case Qt::PermissionStatus::Denied:
        qDebug() << "Bluetooth permission denied";
        emit errorOccurred("Bluetooth permission required. Please enable in Settings.");
        return;
    case Qt::PermissionStatus::Granted:
        qDebug() << "Bluetooth permission already granted";
        break;
    }
#endif
    doStartScan();
}

void BLEManager::doStartScan() {
    clearDevices();
    m_scanning = true;
    emit scanningChanged();

    // Scan for BLE devices only
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BLEManager::stopScan() {
    if (!m_scanning) return;

    m_discoveryAgent->stop();
    m_scanning = false;
    emit scanningChanged();
}

void BLEManager::clearDevices() {
    m_de1Devices.clear();
    m_scales.clear();
    emit devicesChanged();
    emit scalesChanged();
}

void BLEManager::onDeviceDiscovered(const QBluetoothDeviceInfo& device) {
    // Check if it's a DE1
    if (isDE1Device(device)) {
        // Avoid duplicates
        for (const auto& existing : m_de1Devices) {
            if (existing.address() == device.address()) {
                return;
            }
        }
        m_de1Devices.append(device);
        emit devicesChanged();
        emit de1Discovered(device);
        return;
    }

    // Check if it's a scale
    QString scaleType = getScaleType(device);
    if (!scaleType.isEmpty()) {
        // Avoid duplicates
        for (const auto& pair : m_scales) {
            if (pair.first.address() == device.address()) {
                return;
            }
        }
        m_scales.append({device, scaleType});
        emit scalesChanged();
        emit scaleDiscovered(device, scaleType);
    }
}

void BLEManager::onScanFinished() {
    m_scanning = false;
    emit scanningChanged();

    // Auto-restart scan if looking for scale and none connected
    if (shouldContinueScanning()) {
        qDebug() << "Scale not connected, will restart scan in 5 seconds...";
        m_rescanTimer->start();
    }
}

void BLEManager::onScanError(QBluetoothDeviceDiscoveryAgent::Error error) {
    QString errorMsg;
    switch (error) {
        case QBluetoothDeviceDiscoveryAgent::PoweredOffError:
            errorMsg = "Bluetooth is powered off";
            break;
        case QBluetoothDeviceDiscoveryAgent::InputOutputError:
            errorMsg = "Bluetooth I/O error";
            break;
        case QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError:
            errorMsg = "Invalid Bluetooth adapter";
            break;
        case QBluetoothDeviceDiscoveryAgent::UnsupportedPlatformError:
            errorMsg = "Platform does not support Bluetooth LE";
            break;
        case QBluetoothDeviceDiscoveryAgent::UnsupportedDiscoveryMethod:
            errorMsg = "Unsupported discovery method";
            break;
        case QBluetoothDeviceDiscoveryAgent::LocationServiceTurnedOffError:
            errorMsg = "Location services are turned off";
            break;
        default:
            errorMsg = "Unknown Bluetooth error";
            break;
    }
    emit errorOccurred(errorMsg);
    m_scanning = false;
    emit scanningChanged();
}

bool BLEManager::isDE1Device(const QBluetoothDeviceInfo& device) const {
    // Check by name
    QString name = device.name();
    if (name.startsWith("DE1", Qt::CaseInsensitive)) {
        return true;
    }

    // Check by service UUID
    QList<QBluetoothUuid> uuids = device.serviceUuids();
    for (const auto& uuid : uuids) {
        if (uuid == DE1::SERVICE_UUID) {
            return true;
        }
    }

    return false;
}

QString BLEManager::getScaleType(const QBluetoothDeviceInfo& device) const {
    ScaleType type = ScaleFactory::detectScaleType(device);
    if (type == ScaleType::Unknown) {
        return "";
    }
    return ScaleFactory::scaleTypeName(type);
}

void BLEManager::setScaleDevice(ScaleDevice* scale) {
    if (m_scaleDevice) {
        disconnect(m_scaleDevice, nullptr, this, nullptr);
    }

    m_scaleDevice = scale;

    if (m_scaleDevice) {
        connect(m_scaleDevice, &ScaleDevice::connectedChanged,
                this, &BLEManager::onScaleConnectedChanged);
    }
}

void BLEManager::setAutoScanForScale(bool enabled) {
    if (m_autoScanForScale != enabled) {
        m_autoScanForScale = enabled;
        emit autoScanForScaleChanged();

        if (enabled && shouldContinueScanning() && !m_scanning) {
            startScan();
        } else if (!enabled) {
            m_rescanTimer->stop();
        }
    }
}

void BLEManager::onScaleConnectedChanged() {
    if (m_scaleDevice && m_scaleDevice->isConnected()) {
        // Scale connected - stop auto-scanning
        qDebug() << "Scale connected, stopping auto-scan";
        m_rescanTimer->stop();
    } else if (shouldContinueScanning() && !m_scanning) {
        // Scale disconnected - restart scanning
        qDebug() << "Scale disconnected, restarting scan";
        startScan();
    }
}

void BLEManager::restartScanForScale() {
    if (shouldContinueScanning()) {
        qDebug() << "Restarting scan for scale...";
        startScan();
    }
}

bool BLEManager::shouldContinueScanning() const {
    if (!m_autoScanForScale) return false;
    if (m_scaleDevice && m_scaleDevice->isConnected()) return false;
    return true;
}

void BLEManager::setSavedScaleAddress(const QString& address, const QString& type) {
    m_savedScaleAddress = address;
    m_savedScaleType = type;
    qDebug() << "Saved scale address:" << address << "type:" << type;
}

void BLEManager::tryDirectConnectToScale() {
    if (m_savedScaleAddress.isEmpty() || m_savedScaleType.isEmpty()) {
        qDebug() << "No saved scale address, cannot try direct connect";
        return;
    }

    if (m_scaleDevice && m_scaleDevice->isConnected()) {
        qDebug() << "Scale already connected";
        return;
    }

    qDebug() << "Trying direct connect to wake scale:" << m_savedScaleAddress;

    // Create a QBluetoothDeviceInfo from the saved address
    // This will attempt to connect even if the scale isn't advertising (wakes it)
    QBluetoothAddress addr(m_savedScaleAddress);
    QBluetoothDeviceInfo deviceInfo(addr, "Saved Scale", 0);

    // Emit as if we discovered it - the handler in main.cpp will create and connect
    emit scaleDiscovered(deviceInfo, m_savedScaleType);
}
