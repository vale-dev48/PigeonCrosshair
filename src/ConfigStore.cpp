#include "ConfigStore.h"
#include <QJsonDocument>
#include <QSettings>

static QSettings makeSettings() {
    return QSettings(QSettings::IniFormat, QSettings::UserScope, "PigeonCrosshair", "PigeonCrosshair");
}

ConfigStore::ConfigStore(QObject* parent) : QObject(parent) {
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &ConfigStore::save);
    load();
}

void ConfigStore::setConfig(const CrosshairConfig& c) {
    m_config = c;
    emit configChanged();
    m_saveTimer.start();
}

void ConfigStore::setMonitorName(const QString& n) {
    if (n == m_monitor) return;
    m_monitor = n;
    emit monitorChanged();
    m_saveTimer.start();
}

void ConfigStore::setOverlayVisible(bool v) {
    if (v == m_visible) return;
    m_visible = v;
    emit visibilityChanged(v);
    m_saveTimer.start();
}

QStringList ConfigStore::builtinNames() const {
    return {"Cross", "Chevron", "Line", "Pigeon", "Dot"};
}

bool ConfigStore::isBuiltin(const QString& name) const { return builtinNames().contains(name); }

QStringList ConfigStore::presetNames() const { return builtinNames() + m_userPresets.keys(); }

bool ConfigStore::applyPreset(const QString& name) {
    const int i = builtinNames().indexOf(name);
    if (i >= 0) {
        CrosshairConfig c = CrosshairConfig::defaultsFor(CrosshairType(i));
        // I preset built-in non toccano immagine e offset dell'utente.
        c.imagePath = m_config.imagePath;
        c.offsetX = m_config.offsetX;
        c.offsetY = m_config.offsetY;
        setConfig(c);
        return true;
    }
    auto it = m_userPresets.constFind(name);
    if (it == m_userPresets.constEnd()) return false;
    setConfig(*it);
    return true;
}

void ConfigStore::saveUserPreset(const QString& name) {
    if (name.trimmed().isEmpty() || isBuiltin(name.trimmed())) return;
    m_userPresets[name.trimmed()] = m_config;
    emit presetsChanged();
    m_saveTimer.start();
}

void ConfigStore::deleteUserPreset(const QString& name) {
    if (m_userPresets.remove(name)) {
        emit presetsChanged();
        m_saveTimer.start();
    }
}

void ConfigStore::load() {
    QSettings s = makeSettings();
    auto parse = [](const QString& str) {
        return CrosshairConfig::fromJson(QJsonDocument::fromJson(str.toUtf8()).object());
    };
    if (s.contains("config")) m_config = parse(s.value("config").toString());
    m_monitor = s.value("monitor").toString();
    m_visible = s.value("overlayVisible", true).toBool();
    s.beginGroup("presets");
    for (const QString& k : s.childKeys()) m_userPresets[k] = parse(s.value(k).toString());
    s.endGroup();
}

void ConfigStore::save() {
    QSettings s = makeSettings();
    auto dump = [](const CrosshairConfig& c) {
        return QString::fromUtf8(QJsonDocument(c.toJson()).toJson(QJsonDocument::Compact));
    };
    s.setValue("config", dump(m_config));
    s.setValue("monitor", m_monitor);
    s.setValue("overlayVisible", m_visible);
    s.remove("presets");
    s.beginGroup("presets");
    for (auto it = m_userPresets.constBegin(); it != m_userPresets.constEnd(); ++it)
        s.setValue(it.key(), dump(it.value()));
    s.endGroup();
    s.sync();
}
