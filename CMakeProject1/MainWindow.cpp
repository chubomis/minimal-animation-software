#include "MainWindow.h"

#include "CanvasWidget.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDockWidget>
#include <QFrame>
#include <QKeySequence>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Animation App - Neon Minimal");
    resize(1200, 760);

    buildCentralCanvas();
    buildShortcuts();
    buildMenus();
    buildTopToolbar();
    buildToolDock();
    buildTimelineDock();
    connectActions();

    statusBar()->showMessage("Ready");
}

void MainWindow::buildCentralCanvas()
{
    QWidget* centerArea = new QWidget();
    centerArea->setObjectName("CenterArea");

    QVBoxLayout* centerLayout = new QVBoxLayout(centerArea);
    centerLayout->setContentsMargins(28, 28, 28, 28);
    centerLayout->setSpacing(0);

    QFrame* canvasFrame = new QFrame();
    canvasFrame->setObjectName("CanvasFrame");
    canvasFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout* canvasFrameLayout = new QVBoxLayout(canvasFrame);
    canvasFrameLayout->setContentsMargins(12, 12, 12, 12);
    canvasFrameLayout->setSpacing(0);

    canvas = new CanvasWidget();
    canvas->setMinimumSize(720, 460);

    canvasFrameLayout->addWidget(canvas);
    centerLayout->addWidget(canvasFrame);

    setCentralWidget(centerArea);
}

void MainWindow::buildShortcuts()
{
    QShortcut* zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
    QShortcut* zoomInShortcutAlt = new QShortcut(QKeySequence("Ctrl+="), this);
    QShortcut* zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
    QShortcut* resetZoomShortcut = new QShortcut(QKeySequence("Ctrl+0"), this);

    connect(zoomInShortcut, &QShortcut::activated, this, [this]() {
        canvas->zoomIn();
        updateZoomStatus();
        });

    connect(zoomInShortcutAlt, &QShortcut::activated, this, [this]() {
        canvas->zoomIn();
        updateZoomStatus();
        });

    connect(zoomOutShortcut, &QShortcut::activated, this, [this]() {
        canvas->zoomOut();
        updateZoomStatus();
        });

    connect(resetZoomShortcut, &QShortcut::activated, this, [this]() {
        canvas->resetZoom();
        statusBar()->showMessage("Zoom reset to 100%");
        });
}

void MainWindow::buildMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("File");

    newAction = fileMenu->addAction("New");
    exitAction = fileMenu->addAction("Exit");

    QMenu* editMenu = menuBar()->addMenu("Edit");

    undoMenuAction = editMenu->addAction("Undo");
    undoMenuAction->setShortcut(QKeySequence("Ctrl+Z"));
    undoMenuAction->setShortcutContext(Qt::WindowShortcut);

    redoMenuAction = editMenu->addAction("Redo");
    redoMenuAction->setShortcut(QKeySequence("Ctrl+Y"));
    redoMenuAction->setShortcutContext(Qt::WindowShortcut);

    addAction(undoMenuAction);
    addAction(redoMenuAction);

    menuBar()->addMenu("View");
}

void MainWindow::buildTopToolbar()
{
    QToolBar* topToolbar = addToolBar("Top Controls");
    topToolbar->setMovable(false);

    undoAction = topToolbar->addAction("Undo");
    redoAction = topToolbar->addAction("Redo");
    playAction = topToolbar->addAction("Play");

    timelineAction = topToolbar->addAction("Timeline");
    timelineAction->setCheckable(true);
    timelineAction->setChecked(false);

    menuBar()->actions().last()->menu()->addAction(timelineAction);

    zoomInAction = topToolbar->addAction("Zoom +");
    zoomOutAction = topToolbar->addAction("Zoom -");
    resetZoomAction = topToolbar->addAction("100%");

    rotate90Action = topToolbar->addAction("Rot 90");
    rotate180Action = topToolbar->addAction("Rot 180");
    resetRotationAction = topToolbar->addAction("Reset Rot");

    topToolbar->addSeparator();
    topToolbar->addWidget(new QLabel("FPS"));

    QSpinBox* fpsBox = new QSpinBox();
    fpsBox->setRange(1, 60);
    fpsBox->setValue(24);
    topToolbar->addWidget(fpsBox);
}

void MainWindow::buildToolDock()
{
    QDockWidget* toolDock = new QDockWidget("TOOLS");
    toolDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    QWidget* toolPanel = new QWidget();
    QVBoxLayout* toolLayout = new QVBoxLayout(toolPanel);
    toolLayout->setContentsMargins(12, 12, 12, 12);
    toolLayout->setSpacing(10);

    brushButton = new QPushButton("BRUSH");
    eraserButton = new QPushButton("ERASER");
    moveButton = new QPushButton("MOVE");
    onionSkinButton = new QPushButton("ONION SKIN");
    colorButton = new QPushButton("COLOUR");
    clearButton = new QPushButton("CLEAR CANVAS");
    clearButton->setObjectName("DangerButton");

    brushButton->setCheckable(true);
    eraserButton->setCheckable(true);
    moveButton->setCheckable(true);
    onionSkinButton->setCheckable(true);
    brushButton->setChecked(true);

    QSpinBox* brushSizeBox = new QSpinBox();
    brushSizeBox->setRange(1, 50);
    brushSizeBox->setValue(6);

    QCheckBox* pressureCheckBox = new QCheckBox("Pressure Sensitivity");
    pressureCheckBox->setChecked(true);

    QSlider* pressureSlider = new QSlider(Qt::Horizontal);
    pressureSlider->setRange(0, 100);
    pressureSlider->setValue(70);

    pressureLabel = new QLabel("Pressure: 1.00");

    toolLayout->addWidget(brushButton);
    toolLayout->addWidget(eraserButton);
    toolLayout->addWidget(moveButton);
    toolLayout->addWidget(onionSkinButton);
    toolLayout->addWidget(colorButton);
    toolLayout->addSpacing(8);
    toolLayout->addWidget(new QLabel("Brush Size"));
    toolLayout->addWidget(brushSizeBox);
    toolLayout->addSpacing(8);
    toolLayout->addWidget(pressureCheckBox);
    toolLayout->addWidget(new QLabel("Pressure Strength"));
    toolLayout->addWidget(pressureSlider);
    toolLayout->addWidget(pressureLabel);
    toolLayout->addSpacing(8);
    toolLayout->addWidget(clearButton);
    toolLayout->addStretch();

    toolDock->setWidget(toolPanel);
    addDockWidget(Qt::LeftDockWidgetArea, toolDock);

    connect(brushSizeBox, &QSpinBox::valueChanged, this, [this](int value) {
        canvas->setBrushSize(value);
        });

    connect(pressureCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        canvas->setPressureSensitivity(checked);

        if (checked)
            statusBar()->showMessage("Pressure sensitivity enabled");
        else
            statusBar()->showMessage("Pressure sensitivity disabled");
        });

    connect(pressureSlider, &QSlider::valueChanged, this, [this](int value) {
        canvas->setPressureStrength(value);
        statusBar()->showMessage("Pressure strength: " + QString::number(value));
        });

    QTimer* pressureTimer = new QTimer(this);

    connect(pressureTimer, &QTimer::timeout, this, [this]() {
        pressureLabel->setText(
            "Pressure: " + QString::number(canvas->getLastPressure(), 'f', 2)
        );
        });

    pressureTimer->start(120);
}

void MainWindow::buildTimelineDock()
{
    timelineDock = new QDockWidget("TIMELINE");
    timelineDock->setFeatures(QDockWidget::NoDockWidgetFeatures);

    timelineList = new QListWidget();
    timelineList->setObjectName("TimelineList");
    timelineList->setViewMode(QListView::IconMode);
    timelineList->setFlow(QListView::LeftToRight);
    timelineList->setWrapping(false);
    timelineList->setMovement(QListView::Static);
    timelineList->setResizeMode(QListView::Adjust);
    timelineList->setGridSize(QSize(120, 70));
    timelineList->setUniformItemSizes(true);
    timelineList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    timelineList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    timelineList->addItem("Frame 1");
    timelineList->addItem("+ Add Frame");
    timelineList->setCurrentRow(0);

    timelineDock->setWidget(timelineList);
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock);
    timelineDock->hide();
}

void MainWindow::connectActions()
{
    canvas->setDrawingStartedCallback([this]() {
        if (timelineDock->isVisible())
        {
            timelineDock->hide();
            timelineAction->setChecked(false);
            statusBar()->showMessage("Timeline hidden while drawing");
        }
        });

    connect(timelineAction, &QAction::toggled, this, [this](bool checked) {
        timelineDock->setVisible(checked);

        if (checked)
            statusBar()->showMessage("Timeline shown");
        else
            statusBar()->showMessage("Timeline hidden");
        });

    connect(timelineDock, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        timelineAction->setChecked(visible);
        });

    connect(zoomInAction, &QAction::triggered, this, [this]() {
        canvas->zoomIn();
        updateZoomStatus();
        });

    connect(zoomOutAction, &QAction::triggered, this, [this]() {
        canvas->zoomOut();
        updateZoomStatus();
        });

    connect(resetZoomAction, &QAction::triggered, this, [this]() {
        canvas->resetZoom();
        statusBar()->showMessage("Zoom reset to 100%");
        });

    connect(rotate90Action, &QAction::triggered, this, [this]() {
        canvas->rotate90();
        updateRotationStatus();
        });

    connect(rotate180Action, &QAction::triggered, this, [this]() {
        canvas->rotate180();
        updateRotationStatus();
        });

    connect(resetRotationAction, &QAction::triggered, this, [this]() {
        canvas->resetRotation();
        statusBar()->showMessage("Rotation reset");
        });

    connect(brushButton, &QPushButton::clicked, this, [this]() {
        setActiveTool(brushButton);
        canvas->setEraserMode(false);
        statusBar()->showMessage("Brush tool selected");
        });

    connect(eraserButton, &QPushButton::clicked, this, [this]() {
        setActiveTool(eraserButton);
        canvas->setEraserMode(true);
        statusBar()->showMessage("Eraser tool selected");
        });

    connect(moveButton, &QPushButton::clicked, this, [this]() {
        setActiveTool(moveButton);
        canvas->setEraserMode(false);
        statusBar()->showMessage("Move tool will be added later");
        });

    connect(onionSkinButton, &QPushButton::clicked, this, [this]() {
        setActiveTool(onionSkinButton);
        canvas->setEraserMode(false);
        statusBar()->showMessage("Onion skin will be added later");
        });

    connect(colorButton, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(Qt::black, this, "Choose Brush Colour");

        if (color.isValid())
        {
            canvas->setBrushColor(color);
            canvas->setEraserMode(false);
            setActiveTool(brushButton);
            statusBar()->showMessage("Brush colour changed");
        }
        });

    connect(clearButton, &QPushButton::clicked, this, [this]() {
        canvas->clearCanvas();
        statusBar()->showMessage("Canvas cleared");
        });

    connect(undoAction, &QAction::triggered, this, [this]() {
        canvas->undo();
        });

    connect(redoAction, &QAction::triggered, this, [this]() {
        canvas->redo();
        });

    connect(undoMenuAction, &QAction::triggered, this, [this]() {
        canvas->undo();
        });

    connect(redoMenuAction, &QAction::triggered, this, [this]() {
        canvas->redo();
        });

    connect(newAction, &QAction::triggered, this, [this]() {
        canvas->clearCanvas();
        canvas->resetZoom();
        canvas->resetRotation();
        statusBar()->showMessage("New project started");
        });

    connect(exitAction, &QAction::triggered, this, [this]() {
        close();
        });

    connect(playAction, &QAction::triggered, this, [this]() {
        statusBar()->showMessage("Playback will be added later");
        });

    connect(timelineList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item->text() == "+ Add Frame")
        {
            int frameNumber = timelineList->count();

            QListWidgetItem* newFrame = new QListWidgetItem(
                "Frame " + QString::number(frameNumber)
            );

            timelineList->insertItem(timelineList->count() - 1, newFrame);
            timelineList->setCurrentItem(newFrame);

            statusBar()->showMessage("Frame added");
        }
        else
        {
            timelineList->setCurrentItem(item);
            statusBar()->showMessage(item->text() + " selected");
        }
        });
}

void MainWindow::setActiveTool(QPushButton* activeButton)
{
    brushButton->setChecked(activeButton == brushButton);
    eraserButton->setChecked(activeButton == eraserButton);
    moveButton->setChecked(activeButton == moveButton);
    onionSkinButton->setChecked(activeButton == onionSkinButton);
}

void MainWindow::updateZoomStatus()
{
    statusBar()->showMessage(
        "Zoom: " + QString::number(canvas->getZoomPercent(), 'f', 0) + "%"
    );
}

void MainWindow::updateRotationStatus()
{
    statusBar()->showMessage(
        "Rotation: " + QString::number(canvas->getRotationDegrees(), 'f', 0) + " degrees"
    );
}