#pragma once
#include <QColor>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

// Riga "etichetta + slider + valore" con passo decimale.
class SliderRow : public QWidget {
    Q_OBJECT
public:
    SliderRow(const QString& label, double min, double max, double step,
              const QString& suffix = {}, QWidget* parent = nullptr);
    double value() const { return m_value; }
    void setValue(double v);

signals:
    void valueChanged(double v);

private:
    void commit(double v, bool fromSlider);
    QString format(double v) const;

    QSlider* m_slider;
    QLineEdit* m_edit;
    double m_step, m_min, m_max, m_value;
    int m_decimals;
    QString m_suffix;
};

// Pulsante-campione colore: clic apre il selettore.
class ColorButton : public QPushButton {
    Q_OBJECT
public:
    explicit ColorButton(QWidget* parent = nullptr);
    QColor color() const { return m_color; }
    void setColor(const QColor& c);

signals:
    void colorChanged(const QColor& c);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QColor m_color{Qt::white};
};
