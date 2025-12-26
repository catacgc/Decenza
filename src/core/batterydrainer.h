#pragma once

#include <QObject>
#include <QThread>
#include <QVector>
#include <atomic>

class CpuWorker : public QThread {
    Q_OBJECT
public:
    explicit CpuWorker(QObject* parent = nullptr) : QThread(parent) {}
    void stop() { m_running = false; }

protected:
    void run() override;

private:
    std::atomic<bool> m_running{true};
};

class BatteryDrainer : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(double cpuLoad READ cpuLoad NOTIFY cpuLoadChanged)

public:
    explicit BatteryDrainer(QObject* parent = nullptr);
    ~BatteryDrainer();

    bool running() const { return m_running; }
    double cpuLoad() const { return m_cpuLoad; }

public slots:
    void start();
    void stop();
    void toggle();

signals:
    void runningChanged();
    void cpuLoadChanged();

private:
    void startCpuWorkers();
    void stopCpuWorkers();

    bool m_running = false;
    double m_cpuLoad = 0.0;
    QVector<CpuWorker*> m_workers;
};
