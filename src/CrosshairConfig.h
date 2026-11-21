#pragma once
#include <QColor>
#include <QJsonObject>
#include <QString>

enum class CrosshairType { Cross, Chevron, Line, Pigeon, Dot, Image };

// Tutti i parametri che definiscono un crosshair (= contenuto di un preset).
// Le unita' sono pixel logici (Qt gestisce il DPI per monitor).
struct CrosshairConfig {
    CrosshairType type = CrosshairType::Cross;

    QColor color{0, 255, 170};
    double opacity = 1.0;          // 0..1

    double size = 10;              // lunghezza braccio / larghezza totale (pigeon)
    double thickness = 2;
    double gap = 4;                // spazio centrale

    bool centerDot = false;
    double dotSize = 2;            // raggio px

    bool outlineEnabled = true;
    double outlineThickness = 1;
    QColor outlineColor{0, 0, 0};

    double scale = 1.0;            // moltiplicatore globale (0.1..5)
    double rotation = 0;           // gradi
    int offsetX = 0;
    int offsetY = 0;

    QString imagePath;             // PNG/SVG (type == Image)
    bool imageTint = false;

    static CrosshairConfig defaultsFor(CrosshairType t);
    static QString typeName(CrosshairType t);
    static QString typeLabel(CrosshairType t);
    static CrosshairType typeFromName(const QString& n);

    QJsonObject toJson() const;
    static CrosshairConfig fromJson(const QJsonObject& o);
};
