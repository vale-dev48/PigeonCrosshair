#include "AppController.h"
#include <QGuiApplication>
#include <QScreen>

AppController::AppController(QObject* parent)
    : QObject(parent), m_store(), m_overlay(), m_settings(&m_store), m_tray(&m_store) {
    connect(&m_store, &ConfigStore::configChanged, this, &AppController::refreshOverlay);
    connect(&m_store, &ConfigStore::monitorChanged, this, &AppController::refreshOverlay);
    connect(&m_store, &ConfigStore::visibilityChanged, this, &AppController::refreshOverlay);

    // Hot-plug monitor, cambio risoluzione/DPI: riposiziona l'overlay.
    auto watch = [this](QScreen* s) {
        connect(s, &QScreen::geometryChanged, this, &AppController::refreshOverlay);
        connect(s, &QScreen::logicalDotsPerInchChanged, this, &AppController::refreshOverlay);
    };
    for (QScreen* s : QGuiApplication::screens()) watch(s);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this, watch](QScreen* s) {
        watch(s);
        refreshOverlay();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &AppController::refreshOverlay);
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &AppController::refreshOverlay);

    connect(&m_tray, &TrayController::settingsRequested, &m_settings, &SettingsWindow::showAndRaise);
    connect(&m_tray, &TrayController::quitRequested, qGuiApp, &QGuiApplication::quit);
}

QScreen* AppController::targetScreen() const {
    const QString name = m_store.monitorName();
    if (!name.isEmpty())
        for (QScreen* s : QGuiApplication::screens())
            if (s->name() == name) return s;
    return QGuiApplication::primaryScreen();   // monitor scollegato -> fallback al primario
}

void AppController::refreshOverlay() {
    m_overlay.apply(m_store.config(), targetScreen());
    m_overlay.setVisible(m_store.overlayVisible());
}

void AppController::start() {
    m_tray.show();
    refreshOverlay();
    // Senza tray non ci sarebbe modo di aprire le impostazioni: le mostriamo subito.
    // Con la tray disponibile, al primo avvio le mostriamo comunque per orientare l'utente.
    m_settings.showAndRaise();
}
