#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QTimer>
#include <memory>

#include "core/settings.h"
#include "ble/blemanager.h"
#include "ble/de1device.h"
#include "ble/scaledevice.h"
#include "ble/scales/scalefactory.h"
#include "machine/machinestate.h"
#include "models/shotdatamodel.h"
#include "controllers/maincontroller.h"

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    app.setOrganizationName("DecentEspresso");
    app.setOrganizationDomain("decentespresso.com");
    app.setApplicationName("DE1 Controller");
    app.setApplicationVersion("1.0.0");

    // Use Material style for modern look
    QQuickStyle::setStyle("Material");

    // Create core objects
    Settings settings;
    BLEManager bleManager;
    DE1Device de1Device;
    std::unique_ptr<ScaleDevice> scale;
    ShotDataModel shotDataModel;
    MachineState machineState(&de1Device);
    MainController mainController(&settings, &de1Device, &machineState, &shotDataModel);

    // Set up QML engine
    QQmlApplicationEngine engine;

    // Auto-connect when DE1 is discovered
    QObject::connect(&bleManager, &BLEManager::de1Discovered,
                     &de1Device, [&de1Device](const QBluetoothDeviceInfo& device) {
        if (!de1Device.isConnected() && !de1Device.isConnecting()) {
            qDebug() << "Auto-connecting to DE1:" << device.name();
            de1Device.connectToDevice(device);
        }
    });

    // Connect to any supported scale when discovered
    QObject::connect(&bleManager, &BLEManager::scaleDiscovered,
                     [&scale, &machineState, &engine](const QBluetoothDeviceInfo& device, const QString& type) {
        // Don't connect if we already have a connected scale
        if (scale && scale->isConnected()) {
            return;
        }

        // Create the appropriate scale type using ScaleFactory
        scale = ScaleFactory::createScale(device);
        if (!scale) {
            qWarning() << "Failed to create scale for type:" << type;
            return;
        }

        qDebug() << "Auto-connecting to" << type << "scale:" << device.name();

        // Connect scale to MachineState for stop-on-weight functionality
        machineState.setScale(scale.get());

        // Log scale weight during shots
        QObject::connect(scale.get(), &ScaleDevice::weightChanged,
                         [&scale, &machineState]() {
            if (scale && machineState.isFlowing()) {
                qDebug().nospace()
                    << "SCALE weight:" << QString::number(scale->weight(), 'f', 1) << "g "
                    << "flow:" << QString::number(scale->flowRate(), 'f', 2) << "g/s";
            }
        });

        // Update QML context when scale is created
        QQmlContext* context = engine.rootContext();
        context->setContextProperty("ScaleDevice", scale.get());

        // Connect to the scale
        scale->connectToDevice(device);
    });

    // Auto-start scanning on launch
    QTimer::singleShot(500, &bleManager, &BLEManager::startScan);

    // Expose C++ objects to QML
    QQmlContext* context = engine.rootContext();
    context->setContextProperty("Settings", &settings);
    context->setContextProperty("BLEManager", &bleManager);
    context->setContextProperty("DE1Device", &de1Device);
    context->setContextProperty("ScaleDevice", scale.get());  // nullptr initially, updated when scale connects
    context->setContextProperty("MachineState", &machineState);
    context->setContextProperty("ShotDataModel", &shotDataModel);
    context->setContextProperty("MainController", &mainController);

    // Register types for QML (use different names to avoid conflict with context properties)
    qmlRegisterUncreatableType<DE1Device>("DE1App", 1, 0, "DE1DeviceType",
        "DE1Device is created in C++");
    qmlRegisterUncreatableType<MachineState>("DE1App", 1, 0, "MachineStateType",
        "MachineState is created in C++");

    // Load main QML file (QTP0001 NEW policy uses /qt/qml/ prefix)
    const QUrl url(u"qrc:/qt/qml/DE1App/qml/main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
