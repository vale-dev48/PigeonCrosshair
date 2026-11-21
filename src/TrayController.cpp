#include "TrayController.h"
#include "ConfigStore.h"
#include "CrosshairRenderer.h"
#include <QAction>
#include <QMenu>

TrayController::TrayController(ConfigStore* store, QObject* parent) : QObject(parent), m_store(store) {
    m_tray.setIcon(QIcon(":/icon.svg"));
    m_tray.setToolTip("PigeonCrosshair");

    m_menu = new QMenu;
    m_toggle = m_menu->addAction("Mostra overlay");
    m_toggle->setCheckable(true);
    m_toggle->setChecked(store->overlayVisible());
    connect(m_toggle, &QAction::toggled, store, &ConfigStore::setOverlayVisible);
    connect(store, &ConfigStore::visibilityChanged, this, [this](bool v) {
        QSignalBlocker b(m_toggle);
        m_toggle->setChecked(v);
    });

    m_presetMenu = m_menu->addMenu("Preset");
    rebuildPresets();
    connect(store, &ConfigStore::presetsChanged, this, &TrayController::rebuildPresets);

    m_menu->addAction("Impostazioni…", this, &TrayController::settingsRequested);
    m_menu->addSeparator();
    m_menu->addAction("Esci", this, &TrayController::quitRequested);
    m_tray.setContextMenu(m_menu);

    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) emit settingsRequested();
    });
}

TrayController::~TrayController() { delete m_menu; }

void TrayController::show() { m_tray.show(); }

void TrayController::rebuildPresets() {
    m_presetMenu->clear();
    for (const QString& n : m_store->presetNames()) {
        m_presetMenu->addAction(n, this, [this, n] { m_store->applyPreset(n); });
    }
}
