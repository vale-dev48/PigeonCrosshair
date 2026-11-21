#include "PreviewWidget.h"
#include "ConfigStore.h"
#include "CrosshairRenderer.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

PreviewWidget::PreviewWidget(ConfigStore* store, QWidget* parent) : QWidget(parent), m_store(store) {
    setMinimumSize(300, 300);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Clic: cambia sfondo di prova");
    connect(store, &ConfigStore::configChanged, this, qOverload<>(&QWidget::update));
}

void PreviewWidget::mousePressEvent(QMouseEvent*) {
    m_bg = (m_bg + 1) % 4;
    update();
}

void PreviewWidget::paintEvent(QPaintEvent*) {
    static const QColor bases[4] = {QColor(24, 27, 35), QColor(225, 228, 235), QColor(40, 110, 60), QColor(120, 40, 45)};
    const QColor base = bases[m_bg];
    const bool light = base.lightness() > 128;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath clip;
    clip.addRoundedRect(r, 12, 12);
    p.setClipPath(clip);

    QLinearGradient grad(r.topLeft(), r.bottomRight());
    grad.setColorAt(0, base.lighter(115));
    grad.setColorAt(1, base.darker(130));
    p.fillRect(r, grad);

    // griglia leggera + assi centrali
    p.setPen(QPen(light ? QColor(0, 0, 0, 18) : QColor(255, 255, 255, 14), 1));
    for (qreal x = r.center().x() - 20 * int(r.width() / 40); x < r.right(); x += 20) p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
    for (qreal y = r.center().y() - 20 * int(r.height() / 40); y < r.bottom(); y += 20) p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
    p.setPen(QPen(light ? QColor(0, 0, 0, 40) : QColor(255, 255, 255, 34), 1, Qt::DashLine));
    p.drawLine(QPointF(r.left(), r.center().y()), QPointF(r.right(), r.center().y()));
    p.drawLine(QPointF(r.center().x(), r.top()), QPointF(r.center().x(), r.bottom()));

    const CrosshairConfig& c = m_store->config();
    const qreal ext = CrosshairRenderer::extent(c);
    const qreal zoom = std::clamp(std::min(width(), height()) * 0.36 / ext, 1.0, 5.0);
    CrosshairRenderer::paint(p, c, r.center() + QPointF(c.offsetX, c.offsetY) * zoom, zoom);

    p.setClipping(false);
    p.setPen(QPen(light ? QColor(0, 0, 0, 50) : QColor(255, 255, 255, 30), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 12, 12);
    p.setPen(light ? QColor(0, 0, 0, 120) : QColor(255, 255, 255, 110));
    p.drawText(rect().adjusted(14, 0, -14, -10), Qt::AlignBottom | Qt::AlignLeft,
               QString("Anteprima  x%1").arg(zoom, 0, 'f', 1));
}
