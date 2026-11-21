#include "AppController.h"
#include <QApplication>
#include <QBuffer>
#include <QDataStream>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QMessageBox>
#include <QStyleHints>


// Strumento di build: genera PNG e ICO (PNG-in-ICO) dall'icona SVG.
//   PigeonCrosshair.exe --export-icon <cartella>
static int exportIcons(const QString& dir) {
    QDir().mkpath(dir);
    QSvgRenderer svg(QStringLiteral(":/icon.svg"));
    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    QList<QByteArray> pngs;
    for (int s : sizes) {
        QImage img(s, s, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        svg.render(&p);
        p.end();
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
        pngs << ba;
        if (s == 256 || s == 64) img.save(QDir(dir).filePath(QString("icon-%1.png").arg(s)));
    }
    QFile f(QDir(dir).filePath("icon.ico"));
    if (!f.open(QIODevice::WriteOnly)) return 1;
    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);
    const quint16 n = quint16(pngs.size());
    ds << quint16(0) << quint16(1) << n;
    quint32 offset = 6 + 16 * n;
    for (int i = 0; i < pngs.size(); ++i) {
        const int s = sizes[i];
        ds << quint8(s >= 256 ? 0 : s) << quint8(s >= 256 ? 0 : s) << quint8(0) << quint8(0)
           << quint16(1) << quint16(32) << quint32(pngs[i].size()) << offset;
        offset += quint32(pngs[i].size());
    }
    for (const QByteArray& b : pngs) ds.writeRawData(b.constData(), b.size());
    return 0;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (const int i = args.indexOf("--export-icon"); i >= 0 && i + 1 < args.size())
        return exportIcons(args.at(i + 1));

    app.setApplicationName("PigeonCrosshair");
    app.setWindowIcon(QIcon(":/icon.svg"));
    app.setOrganizationName("PigeonCrosshair");
    app.setQuitOnLastWindowClosed(false);   // vive in tray anche con tutte le finestre chiuse
    app.setStyle("Fusion");
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

    QFile qss(":/style.qss");
    if (qss.open(QIODevice::ReadOnly)) app.setStyleSheet(QString::fromUtf8(qss.readAll()));

    // Una sola istanza.
    QLockFile lock(QDir::temp().filePath("PigeonCrosshair.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(100)) {
        QMessageBox::information(nullptr, "PigeonCrosshair",
                                 "PigeonCrosshair è già in esecuzione (guarda la tray di sistema).");
        return 0;
    }

    AppController controller;
    controller.start();
    return app.exec();
}
