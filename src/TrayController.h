#pragma once
#include <QObject>
#include <QSystemTrayIcon>

class ConfigStore;
class QAction;
class QMenu;

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(ConfigStore* store, QObject* parent = nullptr);
    ~TrayController() override;
    void show();
    bool isAvailable() const { return QSystemTrayIcon::isSystemTrayAvailable(); }

signals:
    void settingsRequested();
    void quitRequested();

private:
    void rebuildPresets();

    ConfigStore* m_store;
    QSystemTrayIcon m_tray;
    QMenu* m_menu;
    QMenu* m_presetMenu;
    QAction* m_toggle;
};
