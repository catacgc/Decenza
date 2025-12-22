#pragma once

#include <QObject>
#include <QList>
#include <QPointF>
#include <QVariantList>

// Marker for phase transitions (shot start, frame changes)
struct PhaseMarker {
    double time;
    QString label;
    int frameNumber;
};

class ShotDataModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList pressureData READ pressureDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList flowData READ flowDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList temperatureData READ temperatureDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList weightData READ weightDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList flowRateData READ flowRateDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList pressureGoalData READ pressureGoalDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList flowGoalData READ flowGoalDataVariant NOTIFY dataChanged)
    Q_PROPERTY(QVariantList temperatureGoalData READ temperatureGoalDataVariant NOTIFY dataChanged)

    Q_PROPERTY(QVariantList phaseMarkers READ phaseMarkersVariant NOTIFY dataChanged)
    Q_PROPERTY(double extractionStartTime READ extractionStartTime NOTIFY dataChanged)

    Q_PROPERTY(double maxTime READ maxTime NOTIFY dataChanged)
    Q_PROPERTY(double maxPressure READ maxPressure NOTIFY dataChanged)
    Q_PROPERTY(double maxFlow READ maxFlow NOTIFY dataChanged)
    Q_PROPERTY(double maxWeight READ maxWeight NOTIFY dataChanged)

public:
    explicit ShotDataModel(QObject* parent = nullptr);

    QList<QPointF> pressureData() const { return m_pressureData; }
    QList<QPointF> flowData() const { return m_flowData; }
    QList<QPointF> temperatureData() const { return m_temperatureData; }
    QList<QPointF> weightData() const { return m_weightData; }
    QList<QPointF> flowRateData() const { return m_flowRateData; }
    QList<QPointF> pressureGoalData() const { return m_pressureGoalData; }
    QList<QPointF> flowGoalData() const { return m_flowGoalData; }
    QList<QPointF> temperatureGoalData() const { return m_temperatureGoalData; }

    QVariantList pressureDataVariant() const;
    QVariantList flowDataVariant() const;
    QVariantList temperatureDataVariant() const;
    QVariantList weightDataVariant() const;
    QVariantList flowRateDataVariant() const;
    QVariantList pressureGoalDataVariant() const;
    QVariantList flowGoalDataVariant() const;
    QVariantList temperatureGoalDataVariant() const;

    double maxTime() const { return m_maxTime; }
    double maxPressure() const { return m_maxPressure; }
    double maxFlow() const { return m_maxFlow; }
    double maxWeight() const { return m_maxWeight; }

    QVariantList phaseMarkersVariant() const;
    double extractionStartTime() const { return m_extractionStartTime; }

public slots:
    void clear();
    void addSample(double time, double pressure, double flow, double temperature,
                   double pressureGoal, double flowGoal, double temperatureGoal,
                   int frameNumber = -1);
    void addWeightSample(double time, double weight, double flowRate);
    void markExtractionStart(double time);
    void addPhaseMarker(double time, const QString& label, int frameNumber = -1);

signals:
    void dataChanged();

private:
    QVariantList toVariantList(const QList<QPointF>& data) const;

    QList<QPointF> m_pressureData;
    QList<QPointF> m_flowData;
    QList<QPointF> m_temperatureData;
    QList<QPointF> m_weightData;
    QList<QPointF> m_flowRateData;
    QList<QPointF> m_pressureGoalData;
    QList<QPointF> m_flowGoalData;
    QList<QPointF> m_temperatureGoalData;

    double m_maxTime = 60.0;
    double m_maxPressure = 12.0;
    double m_maxFlow = 8.0;
    double m_maxWeight = 50.0;

    // Phase tracking
    QList<PhaseMarker> m_phaseMarkers;
    double m_extractionStartTime = -1;
    int m_lastFrameNumber = -1;

    static const int MAX_SAMPLES = 600;  // 2 minutes at 5Hz
};
