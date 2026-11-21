#pragma once
#include "CrosshairConfig.h"
#include <QMap>
#include <QObject>
#include <QTimer>

// Stato applicazione: crosshair corrente, monitor, visibilita', preset. Persistito su .ini.
class ConfigStore : public QObject {
    Q_OBJECT
public:
    explicit ConfigStore(QObject* parent = nullptr);

    const CrosshairConfig& config() const { return m_config; }
    void setConfig(const CrosshairConfig& c);

    QString monitorName() const { return m_monitor; }   // vuoto = monitor primario
    void setMonitorName(const QString& n);

    bool overlayVisible() const { return m_visible; }
    void setOverlayVisible(bool v);

    QStringList presetNames() const;                    // built-in + utente
    bool isBuiltin(const QString& name) const;
    bool applyPreset(const QString& name);
    void saveUserPreset(const QString& name);
    void deleteUserPreset(const QString& name);

signals:
    void configChanged();
    void monitorChanged();
    void visibilityChanged(bool visible);
    void presetsChanged();

private:
    void load();
    void save();
    QStringList builtinNames() const;

    CrosshairConfig m_config;
    QString m_monitor;
    bool m_visible = true;
    QMap<QString, CrosshairConfig> m_userPresets;
    QTimer m_saveTimer;
};
