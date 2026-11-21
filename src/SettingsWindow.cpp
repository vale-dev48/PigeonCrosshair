#include "SettingsWindow.h"
#include "ConfigStore.h"
#include "CrosshairRenderer.h"
#include "PreviewWidget.h"
#include "Widgets.h"
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QIcon typeIcon(CrosshairType t, qreal dpr) {
    if (t == CrosshairType::Image) {
        QPixmap pm(QSize(40, 40) * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(200, 210, 230), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(7, 9, 26, 22), 3, 3);
        p.setBrush(QColor(200, 210, 230));
        p.drawEllipse(QPointF(15, 17), 3, 3);
        QPolygonF mt;
        mt << QPointF(9, 29) << QPointF(18, 20) << QPointF(24, 26) << QPointF(28, 22) << QPointF(31, 29);
        p.setPen(Qt::NoPen);
        p.drawPolygon(mt);
        return QIcon(pm);
    }
    CrosshairConfig c = CrosshairConfig::defaultsFor(t);
    c.color = QColor(235, 240, 250);
    c.outlineEnabled = false;
    return QIcon(CrosshairRenderer::renderPixmap(c, 40, dpr));
}
} // namespace

SettingsWindow::SettingsWindow(ConfigStore* store, QWidget* parent) : QWidget(parent), m_store(store) {
    setObjectName("root");
    setWindowTitle("PigeonCrosshair");
    setWindowIcon(QIcon(":/icon.svg"));
    resize(980, 720);
    build();
    rebuildMonitors();
    rebuildPresets();
    sync();

    connect(store, &ConfigStore::configChanged, this, &SettingsWindow::sync);
    connect(store, &ConfigStore::presetsChanged, this, &SettingsWindow::rebuildPresets);
    connect(store, &ConfigStore::visibilityChanged, this, [this](bool v) {
        QSignalBlocker b(m_overlayToggle);
        m_overlayToggle->setChecked(v);
    });
    connect(store, &ConfigStore::monitorChanged, this, &SettingsWindow::rebuildMonitors);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &SettingsWindow::rebuildMonitors);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &SettingsWindow::rebuildMonitors);
}

void SettingsWindow::showAndRaise() {
    show();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    raise();
    activateWindow();
}

void SettingsWindow::closeEvent(QCloseEvent* e) {
    hide();            // resta in tray
    e->ignore();
}

void SettingsWindow::edit(const std::function<void(CrosshairConfig&)>& fn) {
    if (m_syncing) return;
    CrosshairConfig c = m_store->config();
    fn(c);
    m_store->setConfig(c);
}

QFrame* SettingsWindow::makeCard(const QString& title, QWidget** body) {
    auto* card = new QFrame;
    card->setObjectName("card");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 14, 16, 16);
    lay->setSpacing(10);
    auto* t = new QLabel(title.toUpper());
    t->setObjectName("cardTitle");
    lay->addWidget(t);
    *body = card;
    return card;
}

void SettingsWindow::build() {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(18);

    // ---- colonna sinistra: titolo + anteprima + interruttore overlay ----
    auto* left = new QVBoxLayout;
    left->setSpacing(12);
    auto* title = new QLabel("PigeonCrosshair");
    title->setObjectName("appTitle");
    left->addWidget(title);
    auto* sub = new QLabel("Overlay mirino sempre in primo piano");
    sub->setObjectName("muted");
    left->addWidget(sub);
    left->addWidget(new PreviewWidget(m_store), 1);

    m_overlayToggle = new QCheckBox("Overlay attivo");
    m_overlayToggle->setChecked(m_store->overlayVisible());
    connect(m_overlayToggle, &QCheckBox::toggled, this, [this](bool v) { m_store->setOverlayVisible(v); });
    left->addWidget(m_overlayToggle);
    root->addLayout(left, 5);

    // ---- colonna destra: card scrollabili ----
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* content = new QWidget;
    auto* col = new QVBoxLayout(content);
    col->setContentsMargins(0, 0, 8, 0);
    col->setSpacing(12);
    scroll->setWidget(content);
    root->addWidget(scroll, 6);

    // Preset
    {
        QWidget* card;
        QFrame* f = makeCard("Preset", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(f->layout());
        auto* row = new QHBoxLayout;
        m_presets = new QComboBox;
        row->addWidget(m_presets, 1);
        auto* save = new QPushButton("Salva come…");
        m_deletePreset = new QPushButton("Elimina");
        row->addWidget(save);
        row->addWidget(m_deletePreset);
        lay->addLayout(row);
        col->addWidget(f);

        connect(m_presets, qOverload<int>(&QComboBox::activated), this,
                [this](int) { m_store->applyPreset(m_presets->currentText()); });
        connect(m_presets, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
            m_deletePreset->setEnabled(!m_store->isBuiltin(m_presets->currentText()));
        });
        connect(save, &QPushButton::clicked, this, [this] {
            bool ok = false;
            const QString name = QInputDialog::getText(this, "Salva preset", "Nome preset:",
                                                       QLineEdit::Normal, {}, &ok).trimmed();
            if (!ok || name.isEmpty()) return;
            if (m_store->isBuiltin(name)) {
                QMessageBox::warning(this, "Salva preset", "Nome riservato a un preset di default.");
                return;
            }
            m_store->saveUserPreset(name);
            m_presets->setCurrentText(name);
        });
        connect(m_deletePreset, &QPushButton::clicked, this, [this] {
            m_store->deleteUserPreset(m_presets->currentText());
        });
    }

    // Aspetto
    {
        QWidget* card;
        QFrame* f = makeCard("Aspetto", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(f->layout());

        auto* types = new QHBoxLayout;
        types->setSpacing(8);
        m_typeGroup = new QButtonGroup(this);
        m_typeGroup->setExclusive(true);
        const qreal dpr = devicePixelRatioF();
        for (int i = 0; i < 6; ++i) {
            auto* b = new QToolButton;
            b->setObjectName("typeBtn");
            b->setCheckable(true);
            b->setIcon(typeIcon(CrosshairType(i), dpr));
            b->setIconSize(QSize(36, 36));
            b->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            b->setMinimumSize(0, 66);
            b->setText(CrosshairConfig::typeLabel(CrosshairType(i)));
            b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            b->setCursor(Qt::PointingHandCursor);
            m_typeGroup->addButton(b, i);
            types->addWidget(b);
        }
        lay->addLayout(types);
        connect(m_typeGroup, &QButtonGroup::idClicked, this, [this](int id) {
            const CrosshairConfig old = m_store->config();
            edit([&](CrosshairConfig& c) {
                if (c.type == CrosshairType(id)) return;
                // Cambiando tipo si caricano i valori di default della forma, preservando
                // colore, opacita', trasformazioni e immagine scelti dall'utente.
                CrosshairConfig d = CrosshairConfig::defaultsFor(CrosshairType(id));
                d.color = old.color;
                d.opacity = old.opacity;
                d.scale = old.scale;
                d.rotation = old.rotation;
                d.offsetX = old.offsetX;
                d.offsetY = old.offsetY;
                d.imagePath = old.imagePath;
                d.imageTint = old.imageTint;
                d.outlineColor = old.outlineColor;
                d.centerDot = old.centerDot;
                c = d;
            });
        });

        auto* colorRow = new QHBoxLayout;
        m_colorLabel = new QLabel("Colore");
        m_colorLabel->setFixedWidth(96);
        m_color = new ColorButton;
        m_tint = new QCheckBox("Tinta immagine");
        colorRow->addWidget(m_colorLabel);
        colorRow->addWidget(m_color);
        colorRow->addWidget(m_tint);
        colorRow->addStretch();
        lay->addLayout(colorRow);
        connect(m_color, &ColorButton::colorChanged, this, [this](const QColor& c) { edit([&](auto& x) { x.color = c; }); });
        connect(m_tint, &QCheckBox::toggled, this, [this](bool v) { edit([&](auto& x) { x.imageTint = v; }); });

        m_opacity = new SliderRow("Opacità", 5, 100, 1, " %");
        lay->addWidget(m_opacity);
        connect(m_opacity, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.opacity = v / 100.0; }); });
        col->addWidget(f);
    }

    // Forma
    {
        QWidget* card;
        m_shapeCard = makeCard("Forma", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(m_shapeCard->layout());
        m_size = new SliderRow("Dimensione", 2, 200, 1, " px");
        m_thickness = new SliderRow("Spessore", 0.5, 20, 0.5, " px");
        m_gap = new SliderRow("Gap", 0, 80, 1, " px");
        m_dot = new QCheckBox("Punto centrale");
        m_dotSize = new SliderRow("Raggio punto", 0.5, 12, 0.5, " px");
        for (QWidget* w : {static_cast<QWidget*>(m_size), static_cast<QWidget*>(m_thickness),
                           static_cast<QWidget*>(m_gap), static_cast<QWidget*>(m_dot),
                           static_cast<QWidget*>(m_dotSize)})
            lay->addWidget(w);
        connect(m_size, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.size = v; }); });
        connect(m_thickness, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.thickness = v; }); });
        connect(m_gap, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.gap = v; }); });
        connect(m_dot, &QCheckBox::toggled, this, [this](bool v) { edit([&](auto& x) { x.centerDot = v; }); });
        connect(m_dotSize, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.dotSize = v; }); });
        col->addWidget(m_shapeCard);
    }

    // Contorno
    {
        QWidget* card;
        m_outlineCard = makeCard("Contorno", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(m_outlineCard->layout());
        auto* row = new QHBoxLayout;
        m_outline = new QCheckBox("Contorno attivo");
        m_outlineColor = new ColorButton;
        row->addWidget(m_outline);
        row->addStretch();
        row->addWidget(m_outlineColor);
        lay->addLayout(row);
        m_outlineThickness = new SliderRow("Spessore", 0.5, 6, 0.5, " px");
        lay->addWidget(m_outlineThickness);
        connect(m_outline, &QCheckBox::toggled, this, [this](bool v) { edit([&](auto& x) { x.outlineEnabled = v; }); });
        connect(m_outlineColor, &ColorButton::colorChanged, this, [this](const QColor& c) { edit([&](auto& x) { x.outlineColor = c; }); });
        connect(m_outlineThickness, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.outlineThickness = v; }); });
        col->addWidget(m_outlineCard);
    }

    // Immagine
    {
        QWidget* card;
        m_imageCard = makeCard("Immagine personalizzata", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(m_imageCard->layout());
        auto* row = new QHBoxLayout;
        auto* browse = new QPushButton("Carica PNG / SVG…");
        browse->setObjectName("accent");
        auto* clear = new QPushButton("Rimuovi");
        m_imageLabel = new QLabel("Nessuna immagine");
        m_imageLabel->setObjectName("muted");
        m_imageLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        row->addWidget(browse);
        row->addWidget(clear);
        lay->addLayout(row);
        lay->addWidget(m_imageLabel);
        connect(browse, &QPushButton::clicked, this, [this] {
            const QString f = QFileDialog::getOpenFileName(this, "Scegli immagine", {},
                                                           "Immagini (*.png *.svg *.svgz *.webp *.jpg *.jpeg *.bmp)");
            if (f.isEmpty()) return;
            edit([&](auto& x) { x.type = CrosshairType::Image; x.imagePath = f; });
        });
        connect(clear, &QPushButton::clicked, this, [this] { edit([&](auto& x) { x.imagePath.clear(); }); });
        col->addWidget(m_imageCard);
    }

    // Trasformazione
    {
        QWidget* card;
        QFrame* f = makeCard("Trasformazione", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(f->layout());
        m_scale = new SliderRow("Scala", 10, 500, 5, " %");
        m_rotation = new SliderRow("Rotazione", -180, 180, 1, " °");
        m_offX = new SliderRow("Offset X", -400, 400, 1, " px");
        m_offY = new SliderRow("Offset Y", -400, 400, 1, " px");
        for (QWidget* w : {static_cast<QWidget*>(m_scale), static_cast<QWidget*>(m_rotation),
                           static_cast<QWidget*>(m_offX), static_cast<QWidget*>(m_offY)})
            lay->addWidget(w);
        auto* reset = new QPushButton("Azzera offset");
        lay->addWidget(reset, 0, Qt::AlignLeft);
        connect(m_scale, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.scale = v / 100.0; }); });
        connect(m_rotation, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.rotation = v; }); });
        connect(m_offX, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.offsetX = int(v); }); });
        connect(m_offY, &SliderRow::valueChanged, this, [this](double v) { edit([&](auto& x) { x.offsetY = int(v); }); });
        connect(reset, &QPushButton::clicked, this, [this] { edit([&](auto& x) { x.offsetX = 0; x.offsetY = 0; }); });
        col->addWidget(f);
    }

    // Monitor
    {
        QWidget* card;
        QFrame* f = makeCard("Monitor", &card);
        auto* lay = qobject_cast<QVBoxLayout*>(f->layout());
        m_monitor = new QComboBox;
        lay->addWidget(m_monitor);
        auto* hint = new QLabel("Il mirino viene centrato sul monitor scelto, con il DPI di quel monitor.");
        hint->setObjectName("muted");
        hint->setWordWrap(true);
        lay->addWidget(hint);
        connect(m_monitor, qOverload<int>(&QComboBox::activated), this, [this](int i) {
            m_store->setMonitorName(m_monitor->itemData(i).toString());
        });
        col->addWidget(f);
    }
    col->addStretch();
}

void SettingsWindow::rebuildMonitors() {
    QSignalBlocker b(m_monitor);
    m_monitor->clear();
    m_monitor->addItem("Monitor primario (automatico)", QString());
    int n = 1;
    for (QScreen* s : QGuiApplication::screens()) {
        const QSize px = s->size() * s->devicePixelRatio();
        m_monitor->addItem(QString("Monitor %1 — %2 × %3 @ %4%%%5")
                               .arg(n++).arg(px.width()).arg(px.height())
                               .arg(int(s->devicePixelRatio() * 100))
                               .arg(s == QGuiApplication::primaryScreen() ? "  (principale)" : ""),
                           s->name());
    }
    int idx = m_monitor->findData(m_store->monitorName());
    m_monitor->setCurrentIndex(idx < 0 ? 0 : idx);
}

void SettingsWindow::rebuildPresets() {
    QSignalBlocker b(m_presets);
    const QString cur = m_presets->currentText();
    m_presets->clear();
    m_presets->addItems(m_store->presetNames());
    const int i = m_presets->findText(cur);
    m_presets->setCurrentIndex(i < 0 ? 0 : i);
    m_deletePreset->setEnabled(!m_store->isBuiltin(m_presets->currentText()));
}

void SettingsWindow::sync() {
    const CrosshairConfig& c = m_store->config();
    m_syncing = true;
    {
        QSignalBlocker b(m_typeGroup);
        if (auto* btn = m_typeGroup->button(int(c.type))) btn->setChecked(true);
    }
    auto setChk = [](QCheckBox* w, bool v) { QSignalBlocker b(w); w->setChecked(v); };
    m_color->setColor(c.color);
    m_outlineColor->setColor(c.outlineColor);
    setChk(m_tint, c.imageTint);
    setChk(m_dot, c.centerDot);
    setChk(m_outline, c.outlineEnabled);
    m_opacity->setValue(c.opacity * 100);
    m_size->setValue(c.size);
    m_thickness->setValue(c.thickness);
    m_gap->setValue(c.gap);
    m_dotSize->setValue(c.dotSize);
    m_outlineThickness->setValue(c.outlineThickness);
    m_scale->setValue(c.scale * 100);
    m_rotation->setValue(c.rotation);
    m_offX->setValue(c.offsetX);
    m_offY->setValue(c.offsetY);
    m_imageLabel->setText(c.imagePath.isEmpty() ? "Nessuna immagine caricata"
                                                : QFileInfo(c.imagePath).fileName());

    const bool img = c.type == CrosshairType::Image;
    m_shapeCard->setVisible(!img);
    m_outlineCard->setVisible(!img);
    m_imageCard->setVisible(img);
    m_tint->setVisible(img);
    const bool dotOnly = c.type == CrosshairType::Dot;
    // Controlli non applicabili: nascosti (non solo disattivati, sembrerebbero rotti).
    m_size->setVisible(!dotOnly);
    m_thickness->setVisible(!dotOnly);
    m_gap->setVisible(c.type != CrosshairType::Pigeon && !dotOnly);
    m_dot->setVisible(!dotOnly);
    m_dotSize->setVisible(c.centerDot || dotOnly);
    m_outlineThickness->setVisible(c.outlineEnabled);
    m_outlineColor->setVisible(c.outlineEnabled);
    m_syncing = false;
}
