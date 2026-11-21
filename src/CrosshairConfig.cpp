#include "CrosshairConfig.h"

static const char* kNames[] = {"cross", "chevron", "line", "pigeon", "dot", "image"};

QString CrosshairConfig::typeName(CrosshairType t) { return kNames[int(t)]; }

QString CrosshairConfig::typeLabel(CrosshairType t) {
    switch (t) {
    case CrosshairType::Cross:   return QStringLiteral("Cross");
    case CrosshairType::Chevron: return QStringLiteral("Chevron");
    case CrosshairType::Line:    return QStringLiteral("Line");
    case CrosshairType::Pigeon:  return QStringLiteral("Pigeon");
    case CrosshairType::Dot:     return QStringLiteral("Dot");
    case CrosshairType::Image:   return QStringLiteral("Immagine");
    }
    return {};
}

CrosshairType CrosshairConfig::typeFromName(const QString& n) {
    for (int i = 0; i < 6; ++i)
        if (n == kNames[i]) return CrosshairType(i);
    return CrosshairType::Cross;
}

CrosshairConfig CrosshairConfig::defaultsFor(CrosshairType t) {
    CrosshairConfig c;
    c.type = t;
    switch (t) {
    case CrosshairType::Cross:   c.size = 10; c.thickness = 2; c.gap = 4;  break;
    case CrosshairType::Chevron: c.size = 22; c.thickness = 3; c.gap = 0;  c.color = QColor(255, 80, 90); break;
    case CrosshairType::Line:    c.size = 24; c.thickness = 3; c.gap = 0;  c.color = QColor(255, 220, 60); break;
    case CrosshairType::Pigeon:  c.size = 56; c.thickness = 2.5; c.gap = 0; c.color = QColor(200, 210, 230); break;
    case CrosshairType::Dot:     c.dotSize = 3; c.centerDot = true; c.color = QColor(255, 255, 255); break;
    case CrosshairType::Image:   c.outlineEnabled = false; break;
    }
    return c;
}

QJsonObject CrosshairConfig::toJson() const {
    return {
        {"type", typeName(type)},
        {"color", color.name(QColor::HexRgb)},
        {"opacity", opacity},
        {"size", size},
        {"thickness", thickness},
        {"gap", gap},
        {"centerDot", centerDot},
        {"dotSize", dotSize},
        {"outlineEnabled", outlineEnabled},
        {"outlineThickness", outlineThickness},
        {"outlineColor", outlineColor.name(QColor::HexRgb)},
        {"scale", scale},
        {"rotation", rotation},
        {"offsetX", offsetX},
        {"offsetY", offsetY},
        {"imagePath", imagePath},
        {"imageTint", imageTint},
    };
}

CrosshairConfig CrosshairConfig::fromJson(const QJsonObject& o) {
    CrosshairConfig c = defaultsFor(typeFromName(o["type"].toString()));
    auto col = [&](const char* k, const QColor& d) {
        QColor q(o[k].toString());
        return q.isValid() ? q : d;
    };
    c.color = col("color", c.color);
    c.outlineColor = col("outlineColor", c.outlineColor);
    c.opacity = o["opacity"].toDouble(c.opacity);
    c.size = o["size"].toDouble(c.size);
    c.thickness = o["thickness"].toDouble(c.thickness);
    c.gap = o["gap"].toDouble(c.gap);
    c.centerDot = o["centerDot"].toBool(c.centerDot);
    c.dotSize = o["dotSize"].toDouble(c.dotSize);
    c.outlineEnabled = o["outlineEnabled"].toBool(c.outlineEnabled);
    c.outlineThickness = o["outlineThickness"].toDouble(c.outlineThickness);
    c.scale = o["scale"].toDouble(c.scale);
    c.rotation = o["rotation"].toDouble(c.rotation);
    c.offsetX = o["offsetX"].toInt(c.offsetX);
    c.offsetY = o["offsetY"].toInt(c.offsetY);
    c.imagePath = o["imagePath"].toString(c.imagePath);
    c.imageTint = o["imageTint"].toBool(c.imageTint);
    return c;
}
