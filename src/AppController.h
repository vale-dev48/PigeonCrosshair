#pragma once
#include "ConfigStore.h"
#include "OverlayWindow.h"
#include "SettingsWindow.h"
#include "TrayController.h"
#include <QObject>

class QScreen;

// Collega store <-> overlay <-> impostazioni <-> tray.
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);
    void start();

private:
    QScreen* targetScreen() const;
    void refreshOverlay();

    ConfigStore m_store;
    OverlayWindow m_overlay;
    SettingsWindow m_settings;
    TrayController m_tray;
};
