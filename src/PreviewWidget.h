#pragma once
#include <QWidget>

class ConfigStore;

// Anteprima live: sfondi di test cliccabili (scuro / chiaro / verde / rosso).
class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(ConfigStore* store, QWidget* parent = nullptr);
    QSize sizeHint() const override { return {380, 380}; }

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private:
    ConfigStore* m_store;
    int m_bg = 0;
};
