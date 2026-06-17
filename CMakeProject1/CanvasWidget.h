#pragma once

#include "Stroke.h"

#include <QColor>
#include <QElapsedTimer>
#include <QEventPoint>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QRegion>
#include <QTransform>
#include <QVector>
#include <QWidget>

#include <functional>

class QGestureEvent;
class QMouseEvent;
class QPainter;
class QPinchGesture;
class QResizeEvent;
class QTabletEvent;
class QTouchEvent;
class QWheelEvent;

struct AnimationFrame
{
    QVector<Stroke> strokes;
    QVector<Stroke> redoStack;
};

class CanvasWidget : public QWidget
{
    Q_OBJECT

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

    void addFrame();
    void selectFrame(int index);
    void clearProject();

    int getFrameCount() const;
    int getSelectedFrameIndex() const;

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
    AnimationFrame& currentFrame();
    const AnimationFrame& currentFrame() const;

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

    void beginStroke(
        const QPointF& position,
        qreal pressure,
        bool shouldErase
    );

    void addPoint(const QPointF& rawPosition, qreal pressure);
    void finishStroke();

    void ensureCanvas();
    void rebuildCanvas();

    qreal applyPressureCurve(qreal pressure) const;

    qreal widthFromPressure(
        const Stroke& stroke,
        qreal pressure
    ) const;

    QPen makePen(const Stroke& stroke, qreal pressure) const;

    void drawDot(
        const Stroke& stroke,
        const QPointF& position,
        qreal pressure
    );

    void drawLatestSegment(const Stroke& stroke);
    void drawFullStroke(QPainter& painter, const Stroke& stroke);

private:
    QVector<AnimationFrame> frames;
    int selectedFrameIndex = 0;

    QImage canvasImage;
    Stroke currentStroke;

    QColor brushColor = QColor("#111827");
    int brushSize = 6;
    bool eraserMode = false;
    bool pressureSensitivityEnabled = true;
    int pressureStrength = 70;
    qreal lastPressure = 1.0;

    bool drawing = false;

    qreal viewScale = 1.0;
    qreal viewRotationDegrees = 0.0;
    QPointF viewOffset = QPointF(0.0, 0.0);

    QElapsedTimer inputClock;
    QElapsedTimer repaintClock;

    qint64 lastTabletEventMs = -10000;
    qint64 lastMultiTouchEventMs = -10000;

    bool multiTouchActive = false;
    bool touchPanning = false;
    QPointF lastTouchCenter = QPointF(0.0, 0.0);
    qreal lastTouchDistance = 0.0;
    qreal lastTouchAngle = 0.0;

    QRegion pendingUpdateRegion;
    bool repaintScheduled = false;

    std::function<void()> drawingStartedCallback;
};