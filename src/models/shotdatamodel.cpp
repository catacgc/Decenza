#include "shotdatamodel.h"

ShotDataModel::ShotDataModel(QObject* parent)
    : QObject(parent)
{
}

void ShotDataModel::clear() {
    m_pressureData.clear();
    m_flowData.clear();
    m_temperatureData.clear();
    m_weightData.clear();
    m_flowRateData.clear();
    m_pressureGoalData.clear();
    m_flowGoalData.clear();
    m_temperatureGoalData.clear();
    m_phaseMarkers.clear();
    m_extractionStartTime = -1;
    m_lastFrameNumber = -1;
    m_maxTime = 60.0;
    emit dataChanged();
}

void ShotDataModel::addSample(double time, double pressure, double flow, double temperature,
                              double pressureGoal, double flowGoal, double temperatureGoal,
                              int frameNumber) {
    // Limit data size
    if (m_pressureData.size() >= MAX_SAMPLES) {
        m_pressureData.removeFirst();
        m_flowData.removeFirst();
        m_temperatureData.removeFirst();
        m_pressureGoalData.removeFirst();
        m_flowGoalData.removeFirst();
        m_temperatureGoalData.removeFirst();
    }

    m_pressureData.append(QPointF(time, pressure));
    m_flowData.append(QPointF(time, flow));
    m_temperatureData.append(QPointF(time, temperature));
    m_pressureGoalData.append(QPointF(time, pressureGoal));
    m_flowGoalData.append(QPointF(time, flowGoal));
    m_temperatureGoalData.append(QPointF(time, temperatureGoal));

    // Frame tracking is now handled by MainController which has access to frame names
    Q_UNUSED(frameNumber);

    // Update max values
    if (time > m_maxTime - 10) m_maxTime = time + 10;
    if (pressure > m_maxPressure) m_maxPressure = pressure + 1;
    if (flow > m_maxFlow) m_maxFlow = flow + 1;

    emit dataChanged();
}

void ShotDataModel::addWeightSample(double time, double weight, double flowRate) {
    // Limit data size
    if (m_weightData.size() >= MAX_SAMPLES) {
        m_weightData.removeFirst();
        m_flowRateData.removeFirst();
    }

    m_weightData.append(QPointF(time, weight));
    m_flowRateData.append(QPointF(time, flowRate));

    // Update max weight
    if (weight > m_maxWeight) m_maxWeight = weight + 10;

    emit dataChanged();
}

QVariantList ShotDataModel::toVariantList(const QList<QPointF>& data) const {
    QVariantList result;
    for (const QPointF& point : data) {
        QVariantMap map;
        map["x"] = point.x();
        map["y"] = point.y();
        result.append(map);
    }
    return result;
}

QVariantList ShotDataModel::pressureDataVariant() const {
    return toVariantList(m_pressureData);
}

QVariantList ShotDataModel::flowDataVariant() const {
    return toVariantList(m_flowData);
}

QVariantList ShotDataModel::temperatureDataVariant() const {
    return toVariantList(m_temperatureData);
}

QVariantList ShotDataModel::weightDataVariant() const {
    return toVariantList(m_weightData);
}

QVariantList ShotDataModel::flowRateDataVariant() const {
    return toVariantList(m_flowRateData);
}

QVariantList ShotDataModel::pressureGoalDataVariant() const {
    return toVariantList(m_pressureGoalData);
}

QVariantList ShotDataModel::flowGoalDataVariant() const {
    return toVariantList(m_flowGoalData);
}

QVariantList ShotDataModel::temperatureGoalDataVariant() const {
    return toVariantList(m_temperatureGoalData);
}

QVariantList ShotDataModel::phaseMarkersVariant() const {
    QVariantList result;
    for (const PhaseMarker& marker : m_phaseMarkers) {
        QVariantMap map;
        map["time"] = marker.time;
        map["label"] = marker.label;
        map["frameNumber"] = marker.frameNumber;
        result.append(map);
    }
    return result;
}

void ShotDataModel::markExtractionStart(double time) {
    m_extractionStartTime = time;

    // Add a marker for extraction start
    PhaseMarker marker;
    marker.time = time;
    marker.label = "Start";
    marker.frameNumber = 0;
    m_phaseMarkers.append(marker);

    emit dataChanged();
}

void ShotDataModel::addPhaseMarker(double time, const QString& label, int frameNumber) {
    PhaseMarker marker;
    marker.time = time;
    marker.label = label;
    marker.frameNumber = frameNumber;
    m_phaseMarkers.append(marker);
    emit dataChanged();
}
