#ifndef CUBEWIDGET_H
#define CUBEWIDGET_H

#endif // CUBEWIDGET_H
#pragma once
#include <QWidget>

class CubeWidget : public QWidget {
    Q_OBJECT
public:
    explicit CubeWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(300, 300);
    }
protected:
    void paintEvent(QPaintEvent *event) override;
};
