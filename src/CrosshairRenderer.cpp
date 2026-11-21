#include "CrosshairRenderer.h"
#include <QSvgRenderer>
#include <QFileInfo>
#include <QTransform>
#include <algorithm>
#include <cmath>

namespace CrosshairRenderer {
namespace {

constexpr double kPi = 3.14159265358979323846;

// ---- sorgente immagine (cache a un elemento, solo thread GUI) ----
struct Source {
    QString path;
    bool ok = false;
    QImage raster;
    QSvgRenderer* svg = nullptr;
    QSizeF natural;
};

Source& loadSource(const QString& path) {
    static Source s;
    if (s.path == path) return s;
    delete s.svg;
    s = Source();
    s.path = path;
    if (path.isEmpty() || !QFileInfo::exists(path)) return s;
    if (path.endsWith(".svg", Qt::CaseInsensitive) || path.endsWith(".svgz", Qt::CaseInsensitive)) {
        auto* r = new QSvgRenderer(path);
        if (r->isValid()) {
            s.svg = r;
            s.natural = QSizeF(r->defaultSize());
            if (s.natural.isEmpty()) s.natural = QSizeF(64, 64);
            s.ok = true;
        } else {
            delete r;
        }
    } else {
        QImage img(path);
        if (!img.isNull()) {
            s.raster = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            s.natural = QSizeF(img.size()) / img.devicePixelRatio();
            s.ok = true;
        }
    }
    return s;
}

QImage renderImage(const QString& path, const QSize& px, bool tint, const QColor& color) {
    static QImage cache;
    static QString kPath;
    static QSize kSize;
    static bool kTint = false;
    static QRgb kCol = 0;
    if (!cache.isNull() && kPath == path && kSize == px && kTint == tint && kCol == color.rgba())
        return cache;
    Source& s = loadSource(path);
    QImage out(px, QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    if (s.svg) s.svg->render(&p, QRectF(QPointF(0, 0), QSizeF(px)));
    else p.drawImage(QRectF(QPointF(0, 0), QSizeF(px)), s.raster);
    if (tint) {
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        QColor c = color;
        c.setAlpha(255);
        p.fillRect(out.rect(), c);
    }
    p.end();
    cache = out;
    kPath = path;
    kSize = px;
    kTint = tint;
    kCol = color.rgba();
    return out;
}

// ---- forme ----
QPainterPath crossPath(const CrosshairConfig& c) {
    const double k = c.scale, t = c.thickness * k, len = c.size * k, g = c.gap * k;
    QPainterPath p;
    p.setFillRule(Qt::WindingFill);
    p.addRect(g, -t / 2, len, t);
    p.addRect(-g - len, -t / 2, len, t);
    p.addRect(-t / 2, -g - len, t, len);
    p.addRect(-t / 2, g, t, len);
    return p;
}

QPainterPath chevronPath(const CrosshairConfig& c) {
    const double k = c.scale, t = c.thickness * k, len = c.size * k, g = c.gap * k;
    const double a = 62.0 * kPi / 180.0;               // angolo dalla verticale: largo e aperto
    const double dx = std::sin(a) * len, dy = std::cos(a) * len;
    const double apexY = -g - (t / 2) / std::sin(a);   // bordo interno della punta a distanza gap dal centro
    QPainterPath line;
    line.moveTo(-dx, apexY + dy);
    line.lineTo(0, apexY);
    line.lineTo(dx, apexY + dy);
    QPainterPathStroker st;
    st.setWidth(t);
    st.setCapStyle(Qt::FlatCap);
    st.setJoinStyle(Qt::MiterJoin);
    st.setMiterLimit(6);
    return st.createStroke(line);
}

QPainterPath linePath(const CrosshairConfig& c) {
    const double k = c.scale, t = c.thickness * k, len = c.size * k, g = c.gap * k;
    QPainterPath p;
    p.setFillRule(Qt::WindingFill);
    if (g <= 0.01) {
        p.addRect(-len / 2, -t / 2, len, t);            // esattamente centrata
    } else {
        p.addRect(g / 2, -t / 2, len / 2, t);
        p.addRect(-g / 2 - len / 2, -t / 2, len / 2, t);
    }
    return p;
}

QPainterPath pigeonPath(const CrosshairConfig& c) {
    const double s = c.size * c.scale / 110.0;         // il disegno e' largo ~110 unita'
    const double legW = std::max(1.2, c.thickness * 110.0 / std::max(1.0, c.size));
    QPainterPath b;
    b.setFillRule(Qt::WindingFill);
    b.addEllipse(QPointF(-4, 6), 34, 22);              // corpo
    b.addEllipse(QPointF(27, -14), 13, 13);            // testa
    auto poly = [&](std::initializer_list<QPointF> pts) {
        QPolygonF pg;
        for (const QPointF& q : pts) pg << q;
        b.addPolygon(pg);
        b.closeSubpath();
    };
    poly({{10, -2}, {34, -4}, {36, 14}, {10, 22}});    // collo/petto
    poly({{38, -18}, {52, -11}, {38, -7}});            // becco
    poly({{-28, 0}, {-54, -8}, {-58, 12}, {-30, 18}}); // coda
    b.addRect(2 - legW / 2, 24, legW, 15);             // zampe
    b.addRect(14 - legW / 2, 24, legW, 15);
    b.addRect(-3, 38, 10, legW);                       // piedi
    b.addRect(9, 38, 10, legW);
    QPainterPath eye;
    eye.addEllipse(QPointF(30, -16), 2.6, 2.6);
    QPainterPath wing;
    wing.moveTo(-34, 6);
    wing.quadTo(-12, -12, 16, 8);
    QPainterPathStroker ws;
    ws.setWidth(legW * 0.9);
    ws.setCapStyle(Qt::RoundCap);
    QPainterPath shape = b.simplified().subtracted(eye).subtracted(ws.createStroke(wing));
    QTransform t;
    t.scale(s, s);
    t.translate(3, -6);                                // porta il centro del corpo in (0,0)
    return t.map(shape);
}

QPainterPath dotPath(const CrosshairConfig& c) {
    // Poligono a 72 lati: simplified() appiattisce le curve in modo grossolano a questa scala.
    const double r = c.dotSize * c.scale;
    QPolygonF pg;
    for (int i = 0; i < 72; ++i) {
        const double a = 2 * kPi * i / 72;
        pg << QPointF(r * std::cos(a), r * std::sin(a));
    }
    QPainterPath p;
    p.addPolygon(pg);
    p.closeSubpath();
    return p;
}

} // namespace

Geometry build(const CrosshairConfig& c) {
    Geometry g;
    QTransform rot;
    rot.rotate(c.rotation);

    if (c.type == CrosshairType::Image) {
        Source& s = loadSource(c.imagePath);
        if (s.ok) {
            const double k = 64.0 * c.scale / std::max(s.natural.width(), s.natural.height());
            QSizeF sz(s.natural.width() * k, s.natural.height() * k);
            g.imageRect = QRectF(QPointF(-sz.width() / 2, -sz.height() / 2), sz);
        }
        return g;
    }

    QPainterPath body;
    switch (c.type) {
    case CrosshairType::Cross:   body = crossPath(c);   break;
    case CrosshairType::Chevron: body = chevronPath(c); break;
    case CrosshairType::Line:    body = linePath(c);    break;
    case CrosshairType::Pigeon:  body = pigeonPath(c);  break;
    default: break;
    }
    if (c.centerDot || c.type == CrosshairType::Dot) body.addPath(dotPath(c));
    body = rot.map(body).simplified();

    if (c.outlineEnabled && c.outlineThickness > 0) {
        QPainterPathStroker st;
        st.setWidth(c.outlineThickness * 2);
        st.setJoinStyle(Qt::MiterJoin);
        st.setMiterLimit(3);
        QPainterPath outer = st.createStroke(body).united(body);
        g.ring = outer.subtracted(body);
    }
    g.body = body;
    return g;
}

qreal extent(const CrosshairConfig& c) {
    Geometry g = build(c);
    QRectF r;
    if (c.type == CrosshairType::Image) {
        QTransform rot;
        rot.rotate(c.rotation);
        r = rot.mapRect(g.imageRect);
    } else {
        r = g.body.boundingRect().united(g.ring.boundingRect());
    }
    const double m = std::max({std::abs(r.left()), std::abs(r.right()), std::abs(r.top()), std::abs(r.bottom())});
    return std::ceil(m) + 2;
}

void paint(QPainter& p, const CrosshairConfig& c, const QPointF& center, qreal zoom) {
    Geometry g = build(c);
    p.save();
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    p.translate(center);
    p.scale(zoom, zoom);
    p.setPen(Qt::NoPen);

    if (c.type == CrosshairType::Image) {
        if (!g.imageRect.isEmpty()) {
            const qreal dpr = p.device()->devicePixelRatioF() * zoom;
            QSize px(std::max(1, int(std::ceil(g.imageRect.width() * dpr))),
                     std::max(1, int(std::ceil(g.imageRect.height() * dpr))));
            QImage img = renderImage(c.imagePath, px, c.imageTint, c.color);
            p.rotate(c.rotation);
            p.setOpacity(c.opacity);
            p.drawImage(g.imageRect, img);
        }
    } else {
        if (!g.ring.isEmpty()) {
            QColor oc = c.outlineColor;
            oc.setAlphaF(c.opacity);
            p.fillPath(g.ring, oc);
        }
        QColor fc = c.color;
        fc.setAlphaF(c.opacity);
        p.fillPath(g.body, fc);
    }
    p.restore();
}

QPixmap renderPixmap(const CrosshairConfig& c, int side, qreal dpr) {
    QPixmap pm(QSize(side, side) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    const qreal zoom = (side * 0.46) / extent(c);
    paint(p, c, QPointF(side / 2.0, side / 2.0), zoom);
    return pm;
}

} // namespace CrosshairRenderer
