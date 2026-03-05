#pragma once
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSlider>

class yslider : public QAbstractSlider {
    Q_OBJECT
public:
    yslider(QWidget* parent = nullptr);
    void getpara(int pxr, int pyr);
    ~yslider();
    void setgroovecolor(QColor color);
    void settracecolor(QColor color);
    void sethandelcolor(QColor color);
    bool isdragging();
    double tvalue { 0.0 };
    // bool under_control(bool control);

private:
    QColor groovecolor = QColor(255, 255, 255), tracecolor = QColor(0, 0, 0),
           handelcolor       = QColor(0, 0, 0);
    QPainterPath *groovepath = nullptr, *tracepath = nullptr, *handelpath = nullptr;
    int w = 0, h = 0, xr = 0, yr = 0, x = 0, y = 0;
    double ratio  = 0;
    bool dragging = false;

signals:
    void dragStarted();
    void dragFinished();

public slots:
    void forward();
    void backward();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};