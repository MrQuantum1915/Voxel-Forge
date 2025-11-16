#include "cubewidget.h"
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>

void CubeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF center(width() / 2.0, height() / 2.0); 
    qreal size = 120.0;
    qreal half = size / 2.0;

    
    QPointF topOffset(0, -half * 0.4);

    QColor cTop1("#9b7dff"), cTop2("#7b61ff");
    QColor cLeft1("#7b61ff"), cLeft2("#6c63ff");
    QColor cRight1("#8b7fff"), cRight2("#6c63ff");

    
    
    QColor glowColor(140, 130, 255, 220); 

    
    
    QRadialGradient rg(center.x(), center.y() - half * 0.5, size * 1.5); 
    rg.setColorAt(0.0, glowColor);
    rg.setColorAt(0.4, QColor(140, 130, 255, 80)); 
    rg.setColorAt(1.0, QColor(140, 130, 255, 0));

    p.setBrush(rg);
    p.setPen(Qt::NoPen);
    
    QRectF glowRect(center.x() - size * 1.2, center.y() - size * 1.2, size * 2.4, size * 2.4);
    p.drawEllipse(glowRect);

    
    QPointF topP1 = center + QPointF(0, -half) + topOffset;
    QPointF topP2 = center + QPointF(half, -half/2.0) + topOffset;
    QPointF topP3 = center + QPointF(0, 0) + topOffset;
    QPointF topP4 = center + QPointF(-half, -half/2.0) + topOffset;
    QPolygonF topPoly({topP1, topP2, topP3, topP4});

    QPointF leftP1 = topP4;
    QPointF leftP2 = topP3;
    QPointF leftP3 = center + QPointF(0, half);
    QPointF leftP4 = center + QPointF(-half, half/2.0);
    QPolygonF leftPoly({leftP1, leftP2, leftP3, leftP4});

    QPointF rightP1 = topP2;
    QPointF rightP2 = topP3;
    QPointF rightP3 = center + QPointF(0, half);
    QPointF rightP4 = center + QPointF(half, half/2.0);
    QPolygonF rightPoly({rightP1, rightP2, rightP3, rightP4});

    
    p.setPen(Qt::NoPen); 

    
    QLinearGradient gLeft(leftP1, leftP3);
    gLeft.setColorAt(0, cLeft1.darker(105));
    gLeft.setColorAt(1, cLeft2.darker(115));
    p.setBrush(gLeft);
    p.drawPolygon(leftPoly);

    
    QLinearGradient gRight(rightP1, rightP3);
    gRight.setColorAt(0, cRight1);
    gRight.setColorAt(1, cRight2.darker(110));
    p.setBrush(gRight);
    p.drawPolygon(rightPoly);

    
    QLinearGradient gTop(topP1, topP3);
    gTop.setColorAt(0, cTop1);
    gTop.setColorAt(1, cTop2);
    p.setBrush(gTop);
    p.drawPolygon(topPoly);
}
