#include "batterydrainer.h"
#include <QDebug>
#include <cmath>

// CPU worker - does heavy math to drain battery
void CpuWorker::run() {
    qDebug() << "CpuWorker: Starting on thread" << QThread::currentThreadId();

    // Mix of integer and floating point work for maximum power draw
    volatile double result = 0.0;
    volatile uint64_t primeCount = 0;

    while (m_running) {
        // Prime number search (integer heavy)
        for (int n = 2; n < 10000 && m_running; n++) {
            bool isPrime = true;
            for (int i = 2; i * i <= n; i++) {
                if (n % i == 0) {
                    isPrime = false;
                    break;
                }
            }
            if (isPrime) primeCount++;
        }

        // Floating point heavy (trig functions, sqrt)
        for (int i = 0; i < 10000 && m_running; i++) {
            result += std::sin(i * 0.001) * std::cos(i * 0.002);
            result += std::sqrt(std::abs(result) + 1.0);
            result += std::tan(i * 0.0001);
            result = std::fmod(result, 1000000.0);  // Prevent overflow
        }

        // Matrix-like operations
        double matrix[4][4];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                matrix[i][j] = std::sin(i + j + result);
            }
        }
        for (int k = 0; k < 100; k++) {
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    double sum = 0;
                    for (int m = 0; m < 4; m++) {
                        sum += matrix[i][m] * matrix[m][j];
                    }
                    matrix[i][j] = std::fmod(sum, 1000.0);
                }
            }
        }
        result += matrix[0][0];
    }

    qDebug() << "CpuWorker: Stopping, result=" << result << "primes=" << primeCount;
}

BatteryDrainer::BatteryDrainer(QObject* parent)
    : QObject(parent)
{
}

BatteryDrainer::~BatteryDrainer() {
    stop();
}

void BatteryDrainer::start() {
    if (m_running) return;

    qDebug() << "BatteryDrainer: Starting battery drain";
    m_running = true;
    emit runningChanged();

    startCpuWorkers();

    m_cpuLoad = 100.0;
    emit cpuLoadChanged();
}

void BatteryDrainer::stop() {
    if (!m_running) return;

    qDebug() << "BatteryDrainer: Stopping battery drain";
    m_running = false;
    emit runningChanged();

    stopCpuWorkers();

    m_cpuLoad = 0.0;
    emit cpuLoadChanged();
}

void BatteryDrainer::toggle() {
    if (m_running) {
        stop();
    } else {
        start();
    }
}

void BatteryDrainer::startCpuWorkers() {
    // Create one worker per CPU core for maximum drain
    int numCores = QThread::idealThreadCount();
    qDebug() << "BatteryDrainer: Starting" << numCores << "CPU workers";

    for (int i = 0; i < numCores; i++) {
        auto* worker = new CpuWorker(this);
        worker->start(QThread::HighPriority);
        m_workers.append(worker);
    }
}

void BatteryDrainer::stopCpuWorkers() {
    qDebug() << "BatteryDrainer: Stopping" << m_workers.size() << "CPU workers";

    // Signal all workers to stop
    for (auto* worker : m_workers) {
        worker->stop();
    }

    // Wait for all to finish
    for (auto* worker : m_workers) {
        worker->wait(1000);
        if (worker->isRunning()) {
            worker->terminate();
            worker->wait(500);
        }
        delete worker;
    }

    m_workers.clear();
}
