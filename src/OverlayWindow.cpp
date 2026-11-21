#include "OverlayWindow.h"
#include "CrosshairRenderer.h"
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QWindow>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

OverlayWindow::OverlayWindow(QWidget* parent) : QWidget(parent) {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus |
                   Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setWindowTitle("PigeonCrosshair Overlay");

    // Alcuni giochi/finestre "rubano" lo z-order: lo riaffermiamo periodicamente.
    m_topmostTimer.setInterval(1000);
    connect(&m_topmostTimer, &QTimer::timeout, this, [this] {
        if (isVisible()) applyNativeFlags();
    });
}

void OverlayWindow::apply(const CrosshairConfig& cfg, QScreen* screen) {
    m_cfg = cfg;
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    // Finestra quadrata di lato pari/dispari in base allo spessore: con spessore
    // dispari il centro cade a meta' pixel -> linee nitide senza sfocatura.
    const int half = int(CrosshairRenderer::extent(cfg));
    const double lineW = cfg.type == CrosshairType::Dot ? cfg.dotSize * 2 : cfg.thickness;
    const bool oddThickness = cfg.type != CrosshairType::Image &&
                              (int(std::lround(lineW * cfg.scale)) % 2 != 0);
    const int side = half * 2 + (oddThickness ? 1 : 0);

    create();                                   // assicura il QWindow nativo
    if (windowHandle() && windowHandle()->screen() != screen)
        windowHandle()->setScreen(screen);      // prima il monitor (DPI corretto) ...

    const QRect sg = screen->geometry();        // coordinate logiche nel desktop virtuale
    const int cx = sg.x() + sg.width() / 2 + cfg.offsetX;
    const int cy = sg.y() + sg.height() / 2 + cfg.offsetY;
    setGeometry(cx - side / 2, cy - side / 2, side, side);   // ... poi la geometria
    update();
    if (isVisible()) applyNativeFlags();
}

void OverlayWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), Qt::transparent);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    CrosshairRenderer::paint(p, m_cfg, QPointF(width() / 2.0, height() / 2.0));
}

void OverlayWindow::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    applyNativeFlags();
    m_topmostTimer.start();
}

void OverlayWindow::applyNativeFlags() {
#ifdef Q_OS_WIN
    HWND h = reinterpret_cast<HWND>(winId());
    LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
    // LAYERED+TRANSPARENT = click-through; TOOLWINDOW = niente taskbar/Alt+Tab; NOACTIVATE = mai focus.
    ex |= WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    SetWindowLongPtrW(h, GWL_EXSTYLE, ex);
    SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
}
