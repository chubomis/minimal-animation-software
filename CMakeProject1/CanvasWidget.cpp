#include "CanvasWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QEventPoint>
#include <QPointingDevice>
#include <QResizeEvent>
#include <QGestureEvent>
#include <QGesture>
#include <QPinchGesture>
#include <QTimer>
#include <QPen>
#include <QRectF>
#include <QSizeF>
#include <QSizePolicy>

#include <algorithm>
#include <cmath>
#include <utility>

CanvasWidget::CanvasWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StaticContents, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_TabletTracking, true);
    setAttribute(Qt::WA_AcceptTouchEvents, true);

    grabGesture(Qt::PinchGesture);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    frames.push_back(AnimationFrame{});
    selectedFrameIndex = 0;

    inputClock.start();
    repaintClock.start();
}

void CanvasWidget::setBrushColor(const QColor& color)
{
    brushColor = color;
}

void CanvasWidget::setBrushSize(int size)
{
    brushSize = size;
}

void CanvasWidget::setEraserMode(bool enabled)
{
    eraserMode = enabled;
}

void CanvasWidget::setPressureSensitivity(bool enabled)
{
    pressureSensitivityEnabled = enabled;
}

void CanvasWidget::setPressureStrength(int value)
{
    pressureStrength = value;
}

void CanvasWidget::setDrawingStartedCallback(std::function<void()> callback)
{
    drawingStartedCallback = std::move(callback);
}

void CanvasWidget::setOnionSkinEnabled(bool enabled)
{
    if (onionSkinEnabled == enabled)
        return;

    onionSkinEnabled = enabled;

    rebuildCanvas();
    update();
}

void CanvasWidget::setOnionSkinRange(int range)
{
    int clampedRange = std::clamp(range, 1, 2);

    if (onionSkinRange == clampedRange)
        return;

    onionSkinRange = clampedRange;

    if (onionSkinEnabled)
    {
        rebuildCanvas();
        update();
    }
}

qreal CanvasWidget::getLastPressure() const
{
    return lastPressure;
}

qreal CanvasWidget::getZoomPercent() const
{
    return viewScale * 100.0;
}

qreal CanvasWidget::getRotationDegrees() const
{
    return viewRotationDegrees;
}

void CanvasWidget::zoomIn()
{
    zoomAt(rect().center(), 1.15);
}

void CanvasWidget::zoomOut()
{
    zoomAt(rect().center(), 1.0 / 1.15);
}

void CanvasWidget::resetZoom()
{
    QPointF anchor = rect().center();
    QPointF canvasPoint = widgetToCanvas(anchor);

    viewScale = 1.0;
    updateViewOffsetKeepingPoint(anchor, canvasPoint);

    update();
}

void CanvasWidget::rotate90()
{
    rotateAt(rect().center(), 90.0);
}

void CanvasWidget::rotate180()
{
    rotateAt(rect().center(), 180.0);
}

void CanvasWidget::resetRotation()
{
    rotateAt(rect().center(), -viewRotationDegrees);
}

AnimationFrame& CanvasWidget::currentFrame()
{
    if (frames.isEmpty())
    {
        frames.push_back(AnimationFrame{});
        selectedFrameIndex = 0;
    }

    if (selectedFrameIndex < 0 || selectedFrameIndex >= frames.size())
    {
        selectedFrameIndex = 0;
    }

    return frames[selectedFrameIndex];
}

const AnimationFrame& CanvasWidget::currentFrame() const
{
    return frames[selectedFrameIndex];
}

void CanvasWidget::addFrame()
{
    if (drawing)
    {
        finishStroke();
    }

    frames.push_back(AnimationFrame{});
    selectedFrameIndex = static_cast<int>(frames.size()) - 1;

    currentStroke.points.clear();
    drawing = false;

    rebuildCanvas();
    update();
}

void CanvasWidget::deleteFrame(int index)
{
    if (frames.size() <= 1)
        return;

    if (index < 0 || index >= frames.size())
        return;

    if (drawing)
    {
        finishStroke();
    }

    frames.removeAt(index);

    if (selectedFrameIndex > index)
    {
        selectedFrameIndex--;
    }
    else if (selectedFrameIndex == index)
    {
        selectedFrameIndex = std::min(index, static_cast<int>(frames.size()) - 1);
    }

    if (selectedFrameIndex < 0)
    {
        selectedFrameIndex = 0;
    }

    currentStroke.points.clear();
    drawing = false;

    rebuildCanvas();
    update();
}

void CanvasWidget::selectFrame(int index)
{
    if (index < 0 || index >= frames.size())
        return;

    if (drawing)
    {
        finishStroke();
    }

    selectedFrameIndex = index;

    currentStroke.points.clear();
    drawing = false;

    rebuildCanvas();
    update();
}

void CanvasWidget::clearProject()
{
    frames.clear();
    frames.push_back(AnimationFrame{});

    selectedFrameIndex = 0;

    currentStroke.points.clear();
    drawing = false;

    rebuildCanvas();
    update();
}

int CanvasWidget::getFrameCount() const
{
    return static_cast<int>(frames.size());
}

int CanvasWidget::getSelectedFrameIndex() const
{
    return selectedFrameIndex;
}

void CanvasWidget::undo()
{
    AnimationFrame& frame = currentFrame();

    if (!frame.strokes.isEmpty())
    {
        frame.redoStack.push_back(frame.strokes.takeLast());
        rebuildCanvas();
        update();
    }
}

void CanvasWidget::redo()
{
    AnimationFrame& frame = currentFrame();

    if (!frame.redoStack.isEmpty())
    {
        frame.strokes.push_back(frame.redoStack.takeLast());
        rebuildCanvas();
        update();
    }
}

void CanvasWidget::clearCanvas()
{
    AnimationFrame& frame = currentFrame();

    frame.strokes.clear();
    frame.redoStack.clear();

    currentStroke.points.clear();
    drawing = false;

    rebuildCanvas();
    update();
}

bool CanvasWidget::event(QEvent* event)
{
    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd ||
        event->type() == QEvent::TouchCancel)
    {
        if (handleTouchEvent(static_cast<QTouchEvent*>(event)))
            return true;
    }

    if (event->type() == QEvent::Gesture)
    {
        return gestureEvent(static_cast<QGestureEvent*>(event));
    }

    return QWidget::event(event);
}

void CanvasWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rebuildCanvas();
}

void CanvasWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#0F1117"));

    if (canvasImage.isNull())
    {
        rebuildCanvas();
    }

    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setTransform(canvasToWidgetTransform());
    painter.drawImage(0, 0, canvasImage);
}

void CanvasWidget::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        const int delta = event->angleDelta().y();

        if (delta > 0)
        {
            zoomAt(event->position(), 1.12);
        }
        else if (delta < 0)
        {
            zoomAt(event->position(), 1.0 / 1.12);
        }

        event->accept();
        return;
    }

    QWidget::wheelEvent(event);
}

void CanvasWidget::mousePressEvent(QMouseEvent* event)
{
    setFocus();

    if (recentTabletInput() || recentMultiTouchInput())
        return;

    if (event->button() == Qt::LeftButton)
    {
        QPointF canvasPosition = widgetToCanvas(event->position());

        if (!isInsideCanvas(canvasPosition))
            return;

        beginStroke(canvasPosition, 1.0, eraserMode);
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (recentTabletInput() || recentMultiTouchInput())
        return;

    if (drawing)
    {
        addPoint(widgetToCanvas(event->position()), 1.0);
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (recentTabletInput() || recentMultiTouchInput())
        return;

    if (event->button() == Qt::LeftButton && drawing)
    {
        addPoint(widgetToCanvas(event->position()), 1.0);
        finishStroke();
    }
}

void CanvasWidget::tabletEvent(QTabletEvent* event)
{
    setFocus();

    if (recentMultiTouchInput())
    {
        event->accept();
        return;
    }

    lastTabletEventMs = inputClock.elapsed();

    qreal pressure = event->pressure();
    pressure = std::clamp(pressure, 0.05, 1.0);
    lastPressure = pressure;

    bool usingStylusEraser =
        event->pointerType() == QPointingDevice::PointerType::Eraser;

    bool shouldErase = eraserMode || usingStylusEraser;

    if (event->type() == QEvent::TabletPress)
    {
        QPointF canvasPosition = widgetToCanvas(event->position());

        if (!isInsideCanvas(canvasPosition))
            return;

        beginStroke(canvasPosition, pressure, shouldErase);
        event->accept();
        return;
    }

    if (event->type() == QEvent::TabletMove && drawing)
    {
        addPoint(widgetToCanvas(event->position()), pressure);
        event->accept();
        return;
    }

    if (event->type() == QEvent::TabletRelease && drawing)
    {
        addPoint(widgetToCanvas(event->position()), pressure);
        finishStroke();
        event->accept();
        return;
    }

    event->ignore();
}

QPointF CanvasWidget::canvasCenter() const
{
    if (canvasImage.isNull())
        return QPointF(width() / 2.0, height() / 2.0);

    return QPointF(canvasImage.width() / 2.0, canvasImage.height() / 2.0);
}

QRectF CanvasWidget::canvasRect() const
{
    if (canvasImage.isNull())
        return QRectF(QPointF(0, 0), QSizeF(size()));

    return QRectF(QPointF(0, 0), QSizeF(canvasImage.size()));
}

QTransform CanvasWidget::canvasToWidgetTransformWith(
    qreal scale,
    qreal rotationDegrees,
    const QPointF& offset
) const
{
    QPointF center = canvasCenter();

    QTransform transform;
    transform.translate(offset.x(), offset.y());
    transform.translate(center.x(), center.y());
    transform.rotate(rotationDegrees);
    transform.scale(scale, scale);
    transform.translate(-center.x(), -center.y());

    return transform;
}

QTransform CanvasWidget::canvasToWidgetTransform() const
{
    return canvasToWidgetTransformWith(viewScale, viewRotationDegrees, viewOffset);
}

void CanvasWidget::updateViewOffsetKeepingPoint(
    const QPointF& widgetPoint,
    const QPointF& canvasPoint
)
{
    QTransform noOffsetTransform =
        canvasToWidgetTransformWith(viewScale, viewRotationDegrees, QPointF(0, 0));

    QPointF mappedWithoutOffset = noOffsetTransform.map(canvasPoint);
    viewOffset = widgetPoint - mappedWithoutOffset;
}

QPointF CanvasWidget::widgetToCanvas(const QPointF& widgetPoint) const
{
    bool invertible = false;
    QTransform inverse = canvasToWidgetTransform().inverted(&invertible);

    if (!invertible)
        return widgetPoint;

    return inverse.map(widgetPoint);
}

bool CanvasWidget::isInsideCanvas(const QPointF& canvasPoint) const
{
    return canvasRect().contains(canvasPoint);
}

bool CanvasWidget::recentTabletInput() const
{
    return inputClock.elapsed() - lastTabletEventMs < 300;
}

bool CanvasWidget::recentMultiTouchInput() const
{
    return multiTouchActive || inputClock.elapsed() - lastMultiTouchEventMs < 250;
}

qreal CanvasWidget::normalizeAngle(qreal angle) const
{
    while (angle > 180.0)
        angle -= 360.0;

    while (angle < -180.0)
        angle += 360.0;

    return angle;
}

qreal CanvasWidget::touchAngleDegrees(const QList<QEventPoint>& points) const
{
    if (points.size() < 2)
        return lastTouchAngle;

    QPointF delta = points[1].position() - points[0].position();
    return std::atan2(delta.y(), delta.x()) * 180.0 / 3.14159265358979323846;
}

QPointF CanvasWidget::averageTouchPosition(const QList<QEventPoint>& points) const
{
    QPointF total(0, 0);

    for (const QEventPoint& point : points)
    {
        total += point.position();
    }

    return total / points.size();
}

qreal CanvasWidget::averageTouchDistance(
    const QList<QEventPoint>& points,
    const QPointF& center
) const
{
    if (points.isEmpty())
        return 0.0;

    qreal totalDistance = 0.0;

    for (const QEventPoint& point : points)
    {
        QPointF delta = point.position() - center;
        totalDistance += std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    }

    return totalDistance / points.size();
}

void CanvasWidget::cancelCurrentStroke()
{
    if (!drawing)
        return;

    currentStroke.points.clear();
    drawing = false;

    rebuildCanvas();
    update();
}

bool CanvasWidget::handleTouchEvent(QTouchEvent* event)
{
    const QList<QEventPoint>& points = event->points();

    if (event->type() == QEvent::TouchEnd ||
        event->type() == QEvent::TouchCancel)
    {
        multiTouchActive = false;
        touchPanning = false;
        lastMultiTouchEventMs = inputClock.elapsed();

        event->accept();
        return true;
    }

    if (points.size() < 2)
    {
        if (multiTouchActive)
        {
            multiTouchActive = false;
            touchPanning = false;
            lastMultiTouchEventMs = inputClock.elapsed();

            event->accept();
            return true;
        }

        return false;
    }

    cancelCurrentStroke();

    multiTouchActive = true;
    lastMultiTouchEventMs = inputClock.elapsed();

    QPointF currentCenter = averageTouchPosition(points);
    qreal currentDistance = averageTouchDistance(points, currentCenter);
    qreal currentAngle = touchAngleDegrees(points);

    if (!touchPanning)
    {
        touchPanning = true;
        lastTouchCenter = currentCenter;
        lastTouchDistance = currentDistance;
        lastTouchAngle = currentAngle;

        event->accept();
        return true;
    }

    QPointF panDelta = currentCenter - lastTouchCenter;

    if (lastTouchDistance > 1.0 && currentDistance > 1.0)
    {
        qreal zoomFactor = currentDistance / lastTouchDistance;
        zoomFactor = std::clamp(zoomFactor, 0.85, 1.18);
        zoomAt(currentCenter, zoomFactor);
    }

    qreal angleDelta = normalizeAngle(currentAngle - lastTouchAngle);

    if (std::abs(angleDelta) > 0.25)
    {
        rotateAt(currentCenter, angleDelta);
    }

    viewOffset += panDelta;

    lastTouchCenter = currentCenter;
    lastTouchDistance = currentDistance;
    lastTouchAngle = currentAngle;

    update();

    event->accept();
    return true;
}

bool CanvasWidget::gestureEvent(QGestureEvent* event)
{
    if (QGesture* pinch = event->gesture(Qt::PinchGesture))
    {
        handlePinchGesture(static_cast<QPinchGesture*>(pinch));
        return true;
    }

    return false;
}

void CanvasWidget::handlePinchGesture(QPinchGesture* gesture)
{
    if (!gesture)
        return;

    if (recentMultiTouchInput())
        return;

    QPointF center = gesture->centerPoint();

    if (gesture->changeFlags() & QPinchGesture::ScaleFactorChanged)
    {
        qreal factor = gesture->scaleFactor();

        if (factor > 0.0)
        {
            factor = std::clamp(factor, 0.85, 1.18);
            zoomAt(center, factor);
        }
    }

    if (gesture->changeFlags() & QPinchGesture::RotationAngleChanged)
    {
        qreal angleDelta = gesture->rotationAngle() - gesture->lastRotationAngle();
        angleDelta = normalizeAngle(angleDelta);

        if (std::abs(angleDelta) > 0.25)
        {
            rotateAt(center, angleDelta);
        }
    }
}

void CanvasWidget::zoomAt(const QPointF& widgetPoint, qreal factor)
{
    if (factor <= 0.0)
        return;

    QPointF canvasPointBeforeZoom = widgetToCanvas(widgetPoint);

    const qreal oldScale = viewScale;
    const qreal newScale = std::clamp(oldScale * factor, 0.25, 5.0);

    if (std::abs(newScale - oldScale) < 0.001)
        return;

    viewScale = newScale;
    updateViewOffsetKeepingPoint(widgetPoint, canvasPointBeforeZoom);

    update();
}

void CanvasWidget::rotateAt(const QPointF& widgetPoint, qreal deltaDegrees)
{
    if (std::abs(deltaDegrees) < 0.001)
        return;

    QPointF canvasPointBeforeRotate = widgetToCanvas(widgetPoint);

    viewRotationDegrees += deltaDegrees;

    while (viewRotationDegrees >= 360.0)
        viewRotationDegrees -= 360.0;

    while (viewRotationDegrees < 0.0)
        viewRotationDegrees += 360.0;

    updateViewOffsetKeepingPoint(widgetPoint, canvasPointBeforeRotate);

    update();
}

void CanvasWidget::requestCanvasUpdate(const QRect& canvasDirtyRect)
{
    QRectF widgetDirtyRect =
        canvasToWidgetTransform().mapRect(QRectF(canvasDirtyRect));

    widgetDirtyRect.adjust(-8, -8, 8, 8);

    pendingUpdateRegion += widgetDirtyRect.toAlignedRect();

    if (repaintClock.elapsed() >= 8)
    {
        update(pendingUpdateRegion);
        pendingUpdateRegion = QRegion();
        repaintClock.restart();
        repaintScheduled = false;
        return;
    }

    if (!repaintScheduled)
    {
        repaintScheduled = true;

        QTimer::singleShot(8, this, [this]() {
            if (!pendingUpdateRegion.isEmpty())
            {
                update(pendingUpdateRegion);
                pendingUpdateRegion = QRegion();
            }

            repaintClock.restart();
            repaintScheduled = false;
            });
    }
}

void CanvasWidget::beginStroke(
    const QPointF& position,
    qreal pressure,
    bool shouldErase
)
{
    if (drawingStartedCallback)
    {
        drawingStartedCallback();
    }

    ensureCanvas();

    currentStroke.points.clear();
    currentStroke.points.reserve(4096);

    currentStroke.color = shouldErase ? QColor("#FAFAFA") : brushColor;
    currentStroke.size = shouldErase ? brushSize * 3 : brushSize;
    currentStroke.eraser = shouldErase;

    currentStroke.points.push_back({ position, pressure });

    drawing = true;
    currentFrame().redoStack.clear();

    drawDot(currentStroke, position, pressure);
}

void CanvasWidget::addPoint(const QPointF& rawPosition, qreal pressure)
{
    if (!drawing || currentStroke.points.isEmpty())
        return;

    if (!isInsideCanvas(rawPosition))
        return;

    QPointF lastPosition = currentStroke.points.last().position;

    const qreal dx = rawPosition.x() - lastPosition.x();
    const qreal dy = rawPosition.y() - lastPosition.y();
    const qreal distance = std::sqrt(dx * dx + dy * dy);

    qreal minDistance = std::max<qreal>(1.5, brushSize * 0.08);

    if (distance < minDistance)
        return;

    QPointF smoothedPosition =
        lastPosition + (rawPosition - lastPosition) * 0.72;

    currentStroke.points.push_back({ smoothedPosition, pressure });

    drawLatestSegment(currentStroke);
}

void CanvasWidget::finishStroke()
{
    if (!drawing)
        return;

    if (currentStroke.points.size() > 1)
    {
        currentFrame().strokes.push_back(currentStroke);
    }

    currentStroke.points.clear();
    drawing = false;
}

void CanvasWidget::ensureCanvas()
{
    if (!canvasImage.isNull())
        return;

    rebuildCanvas();
}

void CanvasWidget::rebuildCanvas()
{
    if (width() <= 0 || height() <= 0)
        return;

    canvasImage = QImage(size(), QImage::Format_ARGB32_Premultiplied);
    canvasImage.fill(QColor("#FAFAFA"));

    QPainter painter(&canvasImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (onionSkinEnabled)
    {
        drawOnionSkinFrames(painter);
    }

    for (const Stroke& stroke : currentFrame().strokes)
    {
        drawFullStroke(painter, stroke);
    }

    if (drawing && currentStroke.points.size() > 1)
    {
        drawFullStroke(painter, currentStroke);
    }
}

void CanvasWidget::drawOnionSkinFrames(QPainter& painter)
{
    if (frames.size() <= 1)
        return;

    const int range = std::clamp(onionSkinRange, 1, 2);

    const QColor previousFrameRed("#EF4444");
    const QColor nextFrameGreen("#22C55E");

    for (int distance = range; distance >= 1; --distance)
    {
        const int previousIndex = selectedFrameIndex - distance;

        if (previousIndex >= 0 && previousIndex < frames.size())
        {
            qreal opacity = distance == 1 ? 0.28 : 0.14;

            for (const Stroke& stroke : frames[previousIndex].strokes)
            {
                drawFullStrokeTinted(painter, stroke, previousFrameRed, opacity);
            }
        }
    }

    for (int distance = range; distance >= 1; --distance)
    {
        const int nextIndex = selectedFrameIndex + distance;

        if (nextIndex >= 0 && nextIndex < frames.size())
        {
            qreal opacity = distance == 1 ? 0.24 : 0.12;

            for (const Stroke& stroke : frames[nextIndex].strokes)
            {
                drawFullStrokeTinted(painter, stroke, nextFrameGreen, opacity);
            }
        }
    }
}

void CanvasWidget::drawFullStrokeTinted(
    QPainter& painter,
    const Stroke& stroke,
    const QColor& tintColor,
    qreal opacity
)
{
    if (stroke.eraser)
        return;

    Stroke tintedStroke = stroke;
    tintedStroke.color = tintColor;
    tintedStroke.eraser = false;

    painter.save();
    painter.setOpacity(std::clamp(opacity, 0.0, 1.0));
    drawFullStroke(painter, tintedStroke);
    painter.restore();
}

qreal CanvasWidget::applyPressureCurve(qreal pressure) const
{
    pressure = std::clamp(pressure, 0.0, 1.0);

    qreal strength = pressureStrength / 100.0;
    qreal sensitivity = 1.0 + strength * 1.5;

    qreal boostedPressure = 0.5 + (pressure - 0.5) * sensitivity;
    boostedPressure = std::clamp(boostedPressure, 0.0, 1.0);

    qreal curvedPressure = std::pow(boostedPressure, 0.85);

    return std::clamp(curvedPressure, 0.0, 1.0);
}

qreal CanvasWidget::widthFromPressure(
    const Stroke& stroke,
    qreal pressure
) const
{
    if (stroke.eraser)
        return stroke.size;

    if (!pressureSensitivityEnabled)
        return stroke.size;

    qreal curvedPressure = applyPressureCurve(pressure);

    qreal minWidth = stroke.size * 0.25;
    qreal maxWidth = stroke.size * 1.85;

    return minWidth + curvedPressure * (maxWidth - minWidth);
}

QPen CanvasWidget::makePen(const Stroke& stroke, qreal pressure) const
{
    QPen pen(stroke.color);
    pen.setWidthF(widthFromPressure(stroke, pressure));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

void CanvasWidget::drawDot(
    const Stroke& stroke,
    const QPointF& position,
    qreal pressure
)
{
    QPainter painter(&canvasImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(makePen(stroke, pressure));
    painter.drawPoint(position);

    const int radius = static_cast<int>(stroke.size * 3);

    requestCanvasUpdate(QRect(
        static_cast<int>(position.x()) - radius,
        static_cast<int>(position.y()) - radius,
        radius * 2,
        radius * 2
    ));
}

void CanvasWidget::drawLatestSegment(const Stroke& stroke)
{
    if (stroke.points.size() < 2)
        return;

    QPainter painter(&canvasImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;

    int i = stroke.points.size() - 1;

    if (i == 1)
    {
        QPointF p0 = stroke.points[0].position;
        QPointF p1 = stroke.points[1].position;

        path.moveTo(p0);
        path.lineTo(p1);
    }
    else
    {
        QPointF p0 = stroke.points[i - 2].position;
        QPointF p1 = stroke.points[i - 1].position;
        QPointF p2 = stroke.points[i].position;

        QPointF mid1 = (p0 + p1) / 2.0;
        QPointF mid2 = (p1 + p2) / 2.0;

        path.moveTo(mid1);
        path.quadTo(p1, mid2);
    }

    qreal averagePressure =
        (stroke.points[i - 1].pressure + stroke.points[i].pressure) / 2.0;

    painter.setPen(makePen(stroke, averagePressure));
    painter.drawPath(path);

    QRectF dirty = path.boundingRect();
    dirty.adjust(
        -stroke.size * 4,
        -stroke.size * 4,
        stroke.size * 4,
        stroke.size * 4
    );

    requestCanvasUpdate(dirty.toAlignedRect());
}

void CanvasWidget::drawFullStroke(QPainter& painter, const Stroke& stroke)
{
    if (stroke.points.isEmpty())
        return;

    if (stroke.points.size() == 1)
    {
        painter.setPen(makePen(stroke, stroke.points[0].pressure));
        painter.drawPoint(stroke.points[0].position);
        return;
    }

    for (int i = 1; i < stroke.points.size(); ++i)
    {
        QPainterPath path;

        if (i == 1)
        {
            QPointF p0 = stroke.points[0].position;
            QPointF p1 = stroke.points[1].position;

            path.moveTo(p0);
            path.lineTo(p1);
        }
        else
        {
            QPointF p0 = stroke.points[i - 2].position;
            QPointF p1 = stroke.points[i - 1].position;
            QPointF p2 = stroke.points[i].position;

            QPointF mid1 = (p0 + p1) / 2.0;
            QPointF mid2 = (p1 + p2) / 2.0;

            path.moveTo(mid1);
            path.quadTo(p1, mid2);
        }

        qreal averagePressure =
            (stroke.points[i - 1].pressure + stroke.points[i].pressure) / 2.0;

        painter.setPen(makePen(stroke, averagePressure));
        painter.drawPath(path);
    }
}