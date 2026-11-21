#pragma once
#include "CrosshairConfig.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

// Disegno unico condiviso da overlay, anteprima e icone: garantisce che
// cio' che vedi nel pannello sia identico a cio' che appare a schermo.
namespace CrosshairRenderer {

struct Geometry {
    QPainterPath body;   // forma piena, centrata su (0,0), gia' scalata e ruotata
    QPainterPath ring;   // solo la fascia del contorno (non sovrapposta al body)
    QRectF imageRect;    // solo per Image (non ruotato)
};

Geometry build(const CrosshairConfig& cfg);

// Raggio (px logici) del quadrato centrato che contiene tutto il disegno.
qreal extent(const CrosshairConfig& cfg);

void paint(QPainter& p, const CrosshairConfig& cfg, const QPointF& center, qreal zoom = 1.0);

// Rende il crosshair adattato (fit) in un pixmap quadrato: icone e tray.
QPixmap renderPixmap(const CrosshairConfig& cfg, int side, qreal dpr = 1.0);

} // namespace CrosshairRenderer
