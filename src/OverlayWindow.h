#pragma once
#include "CrosshairConfig.h"
#include <QTimer>
#include <QWidget>

class QScreen;

// Finestra overlay: senza bordi, trasparente, sempre in primo piano, click-through.
// E' grande solo quanto serve al crosshair e centrata sul monitor scelto (+ offset).
class OverlayWindow : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWindow(QWidget* parent = nullptr);

    // Applica config e posiziona sul monitor indicato (nullptr = primario).
    void apply(const CrosshairConfig& cfg, QScreen* screen);

protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;

private:
    void applyNativeFlags();   // WinAPI: topmost + ex-style click-through

    CrosshairConfig m_cfg;
    QTimer m_topmostTimer;
};
