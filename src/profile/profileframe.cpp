#include "profileframe.h"
#include "../ble/protocol/de1characteristics.h"
#include <QRegularExpression>

QJsonObject ProfileFrame::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["temperature"] = temperature;
    obj["sensor"] = sensor;
    obj["pump"] = pump;
    obj["transition"] = transition;
    obj["pressure"] = pressure;
    obj["flow"] = flow;
    obj["seconds"] = seconds;
    obj["volume"] = volume;

    if (exitIf) {
        obj["exit_if"] = true;
        obj["exit_type"] = exitType;
        obj["exit_pressure_over"] = exitPressureOver;
        obj["exit_pressure_under"] = exitPressureUnder;
        obj["exit_flow_over"] = exitFlowOver;
        obj["exit_flow_under"] = exitFlowUnder;
    }

    if (maxFlowOrPressure > 0) {
        obj["max_flow_or_pressure"] = maxFlowOrPressure;
        obj["max_flow_or_pressure_range"] = maxFlowOrPressureRange;
    }

    return obj;
}

ProfileFrame ProfileFrame::fromJson(const QJsonObject& json) {
    ProfileFrame frame;
    frame.name = json["name"].toString();
    frame.temperature = json["temperature"].toDouble(93.0);
    frame.sensor = json["sensor"].toString("coffee");
    frame.pump = json["pump"].toString("pressure");
    frame.transition = json["transition"].toString("fast");
    frame.pressure = json["pressure"].toDouble(9.0);
    frame.flow = json["flow"].toDouble(2.0);
    frame.seconds = json["seconds"].toDouble(30.0);
    frame.volume = json["volume"].toDouble(0.0);

    frame.exitIf = json["exit_if"].toBool(false);
    frame.exitType = json["exit_type"].toString();
    frame.exitPressureOver = json["exit_pressure_over"].toDouble(0.0);
    frame.exitPressureUnder = json["exit_pressure_under"].toDouble(0.0);
    frame.exitFlowOver = json["exit_flow_over"].toDouble(0.0);
    frame.exitFlowUnder = json["exit_flow_under"].toDouble(0.0);

    frame.maxFlowOrPressure = json["max_flow_or_pressure"].toDouble(0.0);
    frame.maxFlowOrPressureRange = json["max_flow_or_pressure_range"].toDouble(0.6);

    return frame;
}

ProfileFrame ProfileFrame::fromTclList(const QString& tclList) {
    // Parse de1app Tcl list format: {key value key value ...}
    // Example: {exit_if 1 flow 2.0 volume 100 transition fast exit_flow_under 0.0
    //           temperature 93.0 name "preinfusion" pressure 1.0 sensor coffee
    //           pump pressure exit_type pressure_over exit_pressure_over 1.5 seconds 10}

    ProfileFrame frame;
    QString cleaned = tclList.trimmed();

    // Remove outer braces if present
    if (cleaned.startsWith('{') && cleaned.endsWith('}')) {
        cleaned = cleaned.mid(1, cleaned.length() - 2);
    }

    // Parse key-value pairs
    // Handle quoted strings and regular values
    // Pattern: word + whitespace + (quoted string OR non-whitespace)
    QRegularExpression re("(\\w+)\\s+(?:\"([^\"]*)\"|([^\\s]+))");
    QRegularExpressionMatchIterator it = re.globalMatch(cleaned);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2).isEmpty() ? match.captured(3) : match.captured(2);

        if (key == "name") {
            frame.name = value;
        } else if (key == "temperature") {
            frame.temperature = value.toDouble();
        } else if (key == "sensor") {
            frame.sensor = value;
        } else if (key == "pump") {
            frame.pump = value;
        } else if (key == "transition") {
            frame.transition = (value == "smooth" || value == "slow") ? "smooth" : "fast";
        } else if (key == "pressure") {
            frame.pressure = value.toDouble();
        } else if (key == "flow") {
            frame.flow = value.toDouble();
        } else if (key == "seconds") {
            frame.seconds = value.toDouble();
        } else if (key == "volume") {
            frame.volume = value.toDouble();
        } else if (key == "exit_if") {
            frame.exitIf = (value == "1" || value == "true");
        } else if (key == "exit_type") {
            frame.exitType = value;
        } else if (key == "exit_pressure_over") {
            frame.exitPressureOver = value.toDouble();
        } else if (key == "exit_pressure_under") {
            frame.exitPressureUnder = value.toDouble();
        } else if (key == "exit_flow_over") {
            frame.exitFlowOver = value.toDouble();
        } else if (key == "exit_flow_under") {
            frame.exitFlowUnder = value.toDouble();
        } else if (key == "max_flow_or_pressure") {
            frame.maxFlowOrPressure = value.toDouble();
        } else if (key == "max_flow_or_pressure_range") {
            frame.maxFlowOrPressureRange = value.toDouble();
        }
        // Note: "weight" key from de1app TCL is ignored - we use global target weight instead
    }

    return frame;
}

ProfileFrame ProfileFrame::withSetpoint(double pressureOrFlow, double temp) const {
    ProfileFrame copy = *this;
    if (copy.pump == "flow") {
        copy.flow = pressureOrFlow;
    } else {
        copy.pressure = pressureOrFlow;
    }
    copy.temperature = temp;
    return copy;
}

uint8_t ProfileFrame::computeFlags() const {
    uint8_t flags = DE1::FrameFlag::IgnoreLimit;  // Default

    // Flow vs pressure control
    if (pump == "flow") {
        flags |= DE1::FrameFlag::CtrlF;
    }

    // Mix temp vs basket temp
    if (sensor == "water") {
        flags |= DE1::FrameFlag::TMixTemp;
    }

    // Smooth transition (interpolate)
    if (transition == "smooth") {
        flags |= DE1::FrameFlag::Interpolate;
    }

    // Exit conditions
    if (exitIf) {
        if (exitType == "pressure_under") {
            flags |= DE1::FrameFlag::DoCompare;
            // DC_GT = 0 (less than), DC_CompF = 0 (pressure)
        } else if (exitType == "pressure_over") {
            flags |= DE1::FrameFlag::DoCompare | DE1::FrameFlag::DC_GT;
        } else if (exitType == "flow_under") {
            flags |= DE1::FrameFlag::DoCompare | DE1::FrameFlag::DC_CompF;
        } else if (exitType == "flow_over") {
            flags |= DE1::FrameFlag::DoCompare | DE1::FrameFlag::DC_GT | DE1::FrameFlag::DC_CompF;
        }
    }

    return flags;
}

double ProfileFrame::getSetVal() const {
    return (pump == "flow") ? flow : pressure;
}

double ProfileFrame::getTriggerVal() const {
    if (!exitIf) return 0.0;

    if (exitType == "pressure_under") return exitPressureUnder;
    if (exitType == "pressure_over") return exitPressureOver;
    if (exitType == "flow_under") return exitFlowUnder;
    if (exitType == "flow_over") return exitFlowOver;

    return 0.0;
}
