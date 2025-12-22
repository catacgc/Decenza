#include "maincontroller.h"
#include "../core/settings.h"
#include "../ble/de1device.h"
#include "../machine/machinestate.h"
#include "../models/shotdatamodel.h"
#include <QDir>
#include <QStandardPaths>

MainController::MainController(Settings* settings, DE1Device* device,
                               MachineState* machineState, ShotDataModel* shotDataModel,
                               QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_device(device)
    , m_machineState(machineState)
    , m_shotDataModel(shotDataModel)
{
    // Connect to shot sample updates
    if (m_device) {
        connect(m_device, &DE1Device::shotSampleReceived,
                this, &MainController::onShotSampleReceived);

        // Upload profile when device connects
        connect(m_device, &DE1Device::connectedChanged, this, [this]() {
            if (m_device->isConnected() && m_currentProfile.mode() == Profile::Mode::FrameBased) {
                qDebug() << "MainController: Device connected, uploading profile:" << m_currentProfile.title();
                uploadCurrentProfile();
            }
        });
    }

    // Connect to machine state events
    if (m_machineState) {
        connect(m_machineState, &MachineState::espressoCycleStarted,
                this, &MainController::onEspressoCycleStarted);
        connect(m_machineState, &MachineState::shotEnded,
                this, &MainController::onShotEnded);
    }

    // Load initial profile
    refreshProfiles();
    if (m_settings) {
        loadProfile(m_settings->currentProfile());
    } else {
        loadDefaultProfile();
    }
}

QString MainController::currentProfileName() const {
    return m_currentProfile.title();
}

double MainController::targetWeight() const {
    return m_currentProfile.targetWeight();
}

void MainController::setTargetWeight(double weight) {
    if (m_currentProfile.targetWeight() != weight) {
        m_currentProfile.setTargetWeight(weight);
        if (m_machineState) {
            m_machineState->setTargetWeight(weight);
        }
        emit targetWeightChanged();
    }
}

QVariantList MainController::availableProfiles() const {
    QVariantList result;
    for (const QString& name : m_availableProfiles) {
        result.append(name);
    }
    return result;
}

void MainController::loadProfile(const QString& profileName) {
    QString path = profilesPath() + "/" + profileName + ".json";

    if (QFile::exists(path)) {
        m_currentProfile = Profile::loadFromFile(path);
    } else {
        // Try built-in profiles
        path = ":/profiles/" + profileName + ".json";
        if (QFile::exists(path)) {
            m_currentProfile = Profile::loadFromFile(path);
        } else {
            loadDefaultProfile();
        }
    }

    if (m_settings) {
        m_settings->setCurrentProfile(profileName);
    }

    if (m_machineState) {
        m_machineState->setTargetWeight(m_currentProfile.targetWeight());
    }

    // Upload to machine if connected (for frame-based mode)
    if (m_currentProfile.mode() == Profile::Mode::FrameBased) {
        uploadCurrentProfile();
    }

    emit currentProfileChanged();
    emit targetWeightChanged();
}

void MainController::refreshProfiles() {
    m_availableProfiles.clear();

    // Scan profiles directory
    QDir profileDir(profilesPath());
    QStringList filters;
    filters << "*.json";

    QStringList files = profileDir.entryList(filters, QDir::Files);
    for (const QString& file : files) {
        m_availableProfiles.append(file.left(file.length() - 5));  // Remove .json
    }

    // Add built-in profiles
    QDir builtInDir(":/profiles");
    files = builtInDir.entryList(filters, QDir::Files);
    for (const QString& file : files) {
        QString name = file.left(file.length() - 5);
        if (!m_availableProfiles.contains(name)) {
            m_availableProfiles.append(name);
        }
    }

    emit profilesChanged();
}

void MainController::uploadCurrentProfile() {
    if (m_device && m_device->isConnected()) {
        m_device->uploadProfile(m_currentProfile);
    }
}

void MainController::applySteamSettings() {
    if (!m_device || !m_device->isConnected() || !m_settings) return;

    // Send shot settings (includes steam temp/timeout)
    m_device->setShotSettings(
        m_settings->steamTemperature(),
        m_settings->steamTimeout(),
        m_settings->waterTemperature(),
        m_settings->waterVolume(),
        93.0  // Group temp (could be from settings too)
    );

    // Send steam flow via MMR
    m_device->writeMMR(0x803828, m_settings->steamFlow());

    qDebug() << "Applied steam settings: temp=" << m_settings->steamTemperature()
             << "timeout=" << m_settings->steamTimeout()
             << "flow=" << m_settings->steamFlow();
}

void MainController::applyHotWaterSettings() {
    if (!m_device || !m_device->isConnected() || !m_settings) return;

    // Send shot settings (includes water temp/volume)
    m_device->setShotSettings(
        m_settings->steamTemperature(),
        m_settings->steamTimeout(),
        m_settings->waterTemperature(),
        m_settings->waterVolume(),
        93.0  // Group temp
    );

    qDebug() << "Applied hot water settings: temp=" << m_settings->waterTemperature()
             << "volume=" << m_settings->waterVolume();
}

void MainController::setSteamTemperatureImmediate(double temp) {
    if (!m_device || !m_device->isConnected() || !m_settings) return;

    m_settings->setSteamTemperature(temp);

    // Send all shot settings with updated temperature
    m_device->setShotSettings(
        temp,
        m_settings->steamTimeout(),
        m_settings->waterTemperature(),
        m_settings->waterVolume(),
        93.0
    );

    qDebug() << "Steam temperature set to:" << temp;
}

void MainController::setSteamFlowImmediate(int flow) {
    if (!m_device || !m_device->isConnected() || !m_settings) return;

    m_settings->setSteamFlow(flow);

    // Send steam flow via MMR (can be changed in real-time)
    m_device->writeMMR(0x803828, flow);

    qDebug() << "Steam flow set to:" << flow;
}

void MainController::setSteamTimeoutImmediate(int timeout) {
    if (!m_device || !m_device->isConnected() || !m_settings) return;

    m_settings->setSteamTimeout(timeout);

    // Send all shot settings with updated timeout
    m_device->setShotSettings(
        m_settings->steamTemperature(),
        timeout,
        m_settings->waterTemperature(),
        m_settings->waterVolume(),
        93.0
    );

    qDebug() << "Steam timeout set to:" << timeout;
}

void MainController::onEspressoCycleStarted() {
    // Clear the graph when entering espresso preheating (new cycle from idle)
    // This preserves preheating data since we only clear at cycle start
    m_shotStartTime = 0;
    m_extractionStarted = false;
    m_lastFrameNumber = -1;
    if (m_shotDataModel) {
        m_shotDataModel->clear();
    }
    qDebug() << "=== ESPRESSO CYCLE STARTED (graph cleared) ===";
}

void MainController::onShotEnded() {
    // Could save shot history here
    // Note: Don't reset m_extractionStarted here - it's reset in onEspressoCycleStarted
    // Resetting here causes duplicate "extraction started" markers when entering Ending phase
}

void MainController::onShotSampleReceived(const ShotSample& sample) {
    if (!m_shotDataModel || !m_machineState) return;

    // Record during preheating and actual shot phases
    MachineState::Phase phase = m_machineState->phase();
    bool isEspressoPhase = (phase == MachineState::Phase::EspressoPreheating ||
                           phase == MachineState::Phase::Preinfusion ||
                           phase == MachineState::Phase::Pouring ||
                           phase == MachineState::Phase::Ending);

    if (!isEspressoPhase) {
        m_shotStartTime = 0;  // Reset for next shot
        m_extractionStarted = false;
        return;
    }

    // First sample of this espresso cycle - set the base time
    if (m_shotStartTime == 0) {
        m_shotStartTime = sample.timer;
        qDebug() << "=== ESPRESSO PREHEATING STARTED ===";
    }

    double time = sample.timer - m_shotStartTime;

    // Mark when extraction actually starts (transition from preheating to preinfusion/pouring)
    bool isExtracting = (phase == MachineState::Phase::Preinfusion ||
                        phase == MachineState::Phase::Pouring ||
                        phase == MachineState::Phase::Ending);

    if (isExtracting && !m_extractionStarted) {
        m_extractionStarted = true;
        m_shotDataModel->markExtractionStart(time);
        qDebug() << "=== EXTRACTION STARTED at" << time << "s ===";
    }

    // Detect frame changes and add markers with frame names from profile
    // Only track during actual extraction phases (not preheating - frame numbers are unreliable then)
    if (isExtracting && sample.frameNumber >= 0 && sample.frameNumber != m_lastFrameNumber) {
        QString frameName;
        int frameIndex = sample.frameNumber;

        // Look up frame name from current profile
        const auto& steps = m_currentProfile.steps();
        if (frameIndex >= 0 && frameIndex < steps.size()) {
            frameName = steps[frameIndex].name;
        }

        // Fall back to frame number if no name
        if (frameName.isEmpty()) {
            frameName = QString("F%1").arg(frameIndex);
        }

        m_shotDataModel->addPhaseMarker(time, frameName, frameIndex);
        m_lastFrameNumber = sample.frameNumber;

        qDebug() << "Frame change:" << frameIndex << "->" << frameName << "at" << time << "s";
    }

    // Add sample data
    m_shotDataModel->addSample(time, sample.groupPressure,
                               sample.groupFlow, sample.headTemp,
                               sample.setPressureGoal, sample.setFlowGoal, sample.setTempGoal,
                               sample.frameNumber);

    // Detailed logging for development (reduce frequency)
    static int logCounter = 0;
    if (++logCounter % 10 == 0) {
        qDebug().nospace()
            << "SHOT [" << QString::number(time, 'f', 1) << "s] "
            << "F#" << sample.frameNumber << " "
            << "P:" << QString::number(sample.groupPressure, 'f', 2) << " "
            << "F:" << QString::number(sample.groupFlow, 'f', 2) << " "
            << "T:" << QString::number(sample.headTemp, 'f', 1);
    }
}

void MainController::onWeightChanged(double weight) {
    if (!m_shotDataModel || !m_machineState) return;

    double time = m_machineState->shotTime();
    // Flow rate would come from scale device
    m_shotDataModel->addWeightSample(time, weight, 0);
}

void MainController::loadDefaultProfile() {
    m_currentProfile = Profile();
    m_currentProfile.setTitle("Default");
    m_currentProfile.setTargetWeight(36.0);

    // Create a simple default profile
    ProfileFrame preinfusion;
    preinfusion.name = "Preinfusion";
    preinfusion.pump = "pressure";
    preinfusion.pressure = 4.0;
    preinfusion.temperature = 93.0;
    preinfusion.seconds = 10.0;
    preinfusion.exitIf = true;
    preinfusion.exitType = "pressure_over";
    preinfusion.exitPressureOver = 3.0;

    ProfileFrame extraction;
    extraction.name = "Extraction";
    extraction.pump = "pressure";
    extraction.pressure = 9.0;
    extraction.temperature = 93.0;
    extraction.seconds = 30.0;

    m_currentProfile.addStep(preinfusion);
    m_currentProfile.addStep(extraction);
    m_currentProfile.setPreinfuseFrameCount(1);
}

QString MainController::profilesPath() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    path += "/profiles";

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return path;
}
