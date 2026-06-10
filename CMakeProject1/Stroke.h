#pragma once

#include <QColor>
#include <QPointF>
#include <QVector>

struct InkPoint
{
    QPointF position;
    qreal pressure = 1.0;
};

struct Stroke
{
    QVector<InkPoint> points;
    QColor color = Qt::black;
    int size = 6;
    bool eraser = false;
};