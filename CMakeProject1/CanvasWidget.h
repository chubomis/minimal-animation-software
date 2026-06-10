#pragma once

#include <QWidget>
#include <QImage>
#include <QColor>
#include <QPointF>
#include <QElapsedTimer>
#include <QRegion>
#include <QTransform>
#include <QVector>

#include <functional>

#include "Stroke.h"
#include <QEventPoint>
#include <QList>
class QEvent;
class QResizeEvent;
class QPaintEvent;
class QWheelEvent;
class QMouseEvent;
class QTabletEvent;
class QTouchEvent;
class QGestureEvent;
class QPinchGesture;
class QPainter;

class CanvasWidget : public QWidget
{
public:
    explicit CanvasWidget(QWidget* parent = nullptr);

    void setBrushColor(const QColor& color);
    void setBrushSize(int size);
    void setEraserMode(bool enabled);
    void setPressureSensitivity(bool enabled);
    void setPressureStrength(int value);
    void setDrawingStartedCallback(std::function<void()> callback);

    qreal getLastPressure() const;
    qreal getZoomPercent() const;
    qreal getRotationDegrees() const;

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void rotate90();
    void rotate180();
    void resetRotation();

    void undo();
    void redo();
    void clearCanvas();

protected:
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void tabletEvent(QTabletEvent* event) override;

private:
    QPointF canvasCenter() const;
    QRectF canvasRect() const;

    QTransform canvasToWidgetTransformWith(
        qreal scale,
        qreal rotationDegrees,
        const QPointF& offset
    ) const;

    QTransform canvasToWidgetTransform() const;

    void updateViewOffsetKeepingPoint(
        const QPointF& widgetPoint,
        const QPointF& canvasPoint
    );

    QPointF widgetToCanvas(const QPointF& widgetPoint) const;
    bool isInsideCanvas(const QPointF& canvasPoint) const;

    bool recentTabletInput() const;
    bool recentMultiTouchInput() const;

    qreal normalizeAngle(qreal angle) const;
    qreal touchAngleDegrees(const QList<QEventPoint>& points) const;

    QPointF averageTouchPosition(const QList<QEventPoint>& points) const;
    qreal averageTouchDistance(
        const QList<QEventPoint>& points,
        const QPointF& center
    ) const;

    void cancelCurrentStroke();
    bool handleTouchEvent(QTouchEvent* event);

    bool gestureEvent(QGestureEvent* event);
    void handlePinchGesture(QPinchGesture* gesture);

    void zoomAt(const QPointF& widgetPoint, qreal factor);
    void rotateAt(const QPointF& widgetPoint, qreal deltaDegrees);

    void requestCanvasUpdate(const QRect& canvasDirtyRect);

    void beginStroke(const QPointF& position, qreal pressure, bool shouldErase);
    void addPoint(const QPointF& rawPosition, qreal pressure);
    void finishStroke();

    void ensureCanvas();
    void rebuildCanvas();

    qreal applyPressureCurve(qreal pressure) const;
    qreal widthFromPressure(const Stroke& stroke, qreal pressure) const;
    QPen makePen(const Stroke& stroke, qreal pressure) const;

    void drawDot(const Stroke& stroke, const QPointF& position, qreal pressure);
    void drawLatestSegment(const Stroke& stroke);
    void drawFullStroke(QPainter& painter, const Stroke& stroke);

private:
    QImage canvasImage;

    QVector<Stroke> strokes;
    QVector<Stroke> redoStack;
    Stroke currentStroke;

    QColor brushColor = Qt::black;
    int brushSize = 6;
    bool drawing = false;
    bool eraserMode = false;

    bool pressureSensitivityEnabled = true;
    int pressureStrength = 70;
    qreal lastPressure = 1.0;

    qreal viewScale = 1.0;
    qreal viewRotationDegrees = 0.0;
    QPointF viewOffset = QPointF(0, 0);

    bool multiTouchActive = false;
    bool touchPanning = false;
    qint64 lastMultiTouchEventMs = -100000;
    QPointF lastTouchCenter = QPointF(0, 0);
    qreal lastTouchDistance = 0.0;
    qreal lastTouchAngle = 0.0;

    QElapsedTimer inputClock;
    qint64 lastTabletEventMs = -100000;

    QRegion pendingUpdateRegion;
    QElapsedTimer repaintClock;
    bool repaintScheduled = false;

    std::function<void()> drawingStartedCallback;
};