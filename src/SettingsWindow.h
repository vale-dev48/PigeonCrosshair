#pragma once
#include "CrosshairConfig.h"
#include <QWidget>
#include <functional>

class ConfigStore;
class SliderRow;
class ColorButton;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QFrame;
class QPushButton;

class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(ConfigStore* store, QWidget* parent = nullptr);
    void showAndRaise();

protected:
    void closeEvent(QCloseEvent*) override;

private:
    void build();
    void sync();                 // store -> widget
    void rebuildMonitors();
    void rebuildPresets();
    void edit(const std::function<void(CrosshairConfig&)>& fn);
    QFrame* makeCard(const QString& title, QWidget** body);

    ConfigStore* m_store;
    bool m_syncing = false;

    QComboBox* m_presets = nullptr;
    QPushButton* m_deletePreset = nullptr;
    QCheckBox* m_overlayToggle = nullptr;
    QButtonGroup* m_typeGroup = nullptr;
    ColorButton* m_color = nullptr;
    QCheckBox* m_tint = nullptr;
    QLabel* m_colorLabel = nullptr;
    SliderRow *m_opacity, *m_size, *m_thickness, *m_gap, *m_dotSize, *m_outlineThickness;
    SliderRow *m_scale, *m_rotation, *m_offX, *m_offY;
    QCheckBox *m_dot, *m_outline;
    ColorButton* m_outlineColor;
    QLabel* m_imageLabel;
    QComboBox* m_monitor;
    QFrame *m_shapeCard, *m_outlineCard, *m_imageCard;
};
