#include "Widgets.h"
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>

SliderRow::SliderRow(const QString& label, double min, double max, double step,
                     const QString& suffix, QWidget* parent)
    : QWidget(parent), m_step(step), m_min(min), m_max(max), m_value(min),
      m_decimals(step < 1 ? (step < 0.1 ? 2 : 1) : 0), m_suffix(suffix) {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(10);

    auto* lbl = new QLabel(label);
    lbl->setFixedWidth(96);
    lay->addWidget(lbl);

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(int(std::lround(min / step)), int(std::lround(max / step)));
    lay->addWidget(m_slider, 1);

    // Campo testo semplice (al posto di QDoubleSpinBox, che con lo stylesheet nascondeva il testo).
    m_edit = new QLineEdit;
    m_edit->setFixedWidth(92);
    m_edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_edit->setText(format(min));
    lay->addWidget(m_edit);

    connect(m_slider, &QSlider::valueChanged, this, [this](int v) { commit(v * m_step, true); });
    connect(m_edit, &QLineEdit::editingFinished, this, [this] {
        QString t = m_edit->text();
        t.remove(m_suffix).replace(',', '.');
        bool ok = false;
        const double d = t.trimmed().toDouble(&ok);
        if (ok) commit(d, false);
        else m_edit->setText(format(m_value));     // testo non valido: ripristina
    });
}

QString SliderRow::format(double v) const {
    return QString::number(v, 'f', m_decimals) + m_suffix;
}

void SliderRow::commit(double v, bool fromSlider) {
    v = std::min(m_max, std::max(m_min, v));
    v = std::round(v / m_step) * m_step;
    m_value = v;
    m_edit->setText(format(v));
    if (!fromSlider) {
        QSignalBlocker b(m_slider);
        m_slider->setValue(int(std::lround(v / m_step)));
    }
    emit valueChanged(v);
}

void SliderRow::setValue(double v) {
    if (std::abs(m_value - v) < 1e-9) return;
    v = std::min(m_max, std::max(m_min, v));
    m_value = v;
    QSignalBlocker b(m_slider);
    m_slider->setValue(int(std::lround(v / m_step)));
    m_edit->setText(format(v));
}

ColorButton::ColorButton(QWidget* parent) : QPushButton(parent) {
    setFixedSize(84, 30);
    setCursor(Qt::PointingHandCursor);
    connect(this, &QPushButton::clicked, this, [this] {
        const QColor c = QColorDialog::getColor(m_color, window(), "Scegli colore");
        if (c.isValid() && c != m_color) {
            setColor(c);
            emit colorChanged(c);
        }
    });
}

void ColorButton::setColor(const QColor& c) {
    m_color = c;
    update();
}

void ColorButton::paintEvent(QPaintEvent* e) {
    QPushButton::paintEvent(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(255, 255, 255, 50));
    p.setBrush(m_color);
    p.drawRoundedRect(QRectF(rect()).adjusted(6, 6, -6, -6), 5, 5);
}
