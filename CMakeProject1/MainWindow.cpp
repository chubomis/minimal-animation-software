#include "MainWindow.h"

#include "CanvasWidget.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDockWidget>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QWidget>

#include <algorithm>

namespace
{
    constexpr int ItemTypeRole = Qt::UserRole;
    constexpr int FrameIndexRole = Qt::UserRole + 1;

    constexpr int TimelineCardWidth = 170;
    constexpr int TimelineCardHeight = 110;

    void rebuildTimelineItems(QListWidget* timelineList, CanvasWidget* canvas, MainWindow* window);

    int playbackIntervalFromFps(int fps)
    {
        fps = std::clamp(fps, 1, 60);
        return std::max(1, 1000 / fps);
    }

    QWidget* createFrameWidget(
        QListWidget* timelineList,
        CanvasWidget* canvas,
        MainWindow* window,
        int frameIndex
    )
    {
        QWidget* outerWidget = new QWidget();
        outerWidget->setObjectName("TimelineItemOuter");
        outerWidget->setFixedSize(TimelineCardWidth - 12, TimelineCardHeight - 12);

        QVBoxLayout* outerLayout = new QVBoxLayout(outerWidget);
        outerLayout->setContentsMargins(6, 6, 6, 6);
        outerLayout->setSpacing(0);

        QFrame* card = new QFrame();
        card->setObjectName("TimelineFrameCard");
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 10);
        cardLayout->setSpacing(6);

        QHBoxLayout* topRow = new QHBoxLayout();
        topRow->setContentsMargins(0, 0, 0, 0);
        topRow->setSpacing(0);

        QPushButton* deleteButton = new QPushButton("X");
        deleteButton->setObjectName("TimelineDeleteButton");

        // This forces it to be text only, not a garbage icon.
        deleteButton->setIcon(QIcon());
        deleteButton->setText("X");

        deleteButton->setFixedSize(24, 24);
        deleteButton->setToolTip("Delete this frame");
        deleteButton->setCursor(Qt::PointingHandCursor);
        deleteButton->setFocusPolicy(Qt::NoFocus);

        QFont deleteFont = deleteButton->font();
        deleteFont.setBold(true);
        deleteFont.setPointSize(10);
        deleteButton->setFont(deleteFont);

        topRow->addStretch();
        topRow->addWidget(deleteButton, 0, Qt::AlignTop | Qt::AlignRight);

        QPushButton* selectFrameButton = new QPushButton("Frame " + QString::number(frameIndex + 1));
        selectFrameButton->setObjectName("TimelineFrameSelectButton");
        selectFrameButton->setCursor(Qt::PointingHandCursor);
        selectFrameButton->setFocusPolicy(Qt::NoFocus);
        selectFrameButton->setMinimumHeight(42);

        cardLayout->addLayout(topRow);
        cardLayout->addStretch();
        cardLayout->addWidget(selectFrameButton);
        cardLayout->addStretch();

        outerLayout->addWidget(card);

        QObject::connect(selectFrameButton, &QPushButton::clicked, window, [timelineList, canvas, window, frameIndex]() {
            canvas->selectFrame(frameIndex);
            timelineList->setCurrentRow(frameIndex);

            window->statusBar()->showMessage(
                "Frame " + QString::number(frameIndex + 1) + " selected"
            );
            });

        QObject::connect(deleteButton, &QPushButton::clicked, window, [timelineList, canvas, window, frameIndex]() {
            if (canvas->getFrameCount() <= 1)
            {
                window->statusBar()->showMessage("You must keep at least one frame");
                return;
            }

            canvas->deleteFrame(frameIndex);
            rebuildTimelineItems(timelineList, canvas, window);

            window->statusBar()->showMessage(
                "Frame " + QString::number(frameIndex + 1) + " deleted"
            );
            });

        return outerWidget;
    }

    QWidget* createAddFrameWidget()
    {
        QWidget* addWidget = new QWidget();
        addWidget->setObjectName("TimelineAddFrameWidget");
        addWidget->setFixedSize(TimelineCardWidth - 12, TimelineCardHeight - 12);

        QVBoxLayout* layout = new QVBoxLayout(addWidget);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(0);

        QLabel* label = new QLabel("+ Add Frame");
        label->setAlignment(Qt::AlignCenter);

        layout->addStretch();
        layout->addWidget(label);
        layout->addStretch();

        return addWidget;
    }

    void addFrameItem(
        QListWidget* timelineList,
        CanvasWidget* canvas,
        MainWindow* window,
        int frameIndex
    )
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(TimelineCardWidth, TimelineCardHeight));
        item->setData(ItemTypeRole, "frame");
        item->setData(FrameIndexRole, frameIndex);

        timelineList->addItem(item);
        timelineList->setItemWidget(
            item,
            createFrameWidget(timelineList, canvas, window, frameIndex)
        );
    }

    void addAddFrameItem(QListWidget* timelineList)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(QSize(TimelineCardWidth, TimelineCardHeight));
        item->setData(ItemTypeRole, "add");

        timelineList->addItem(item);
        timelineList->setItemWidget(item, createAddFrameWidget());
    }

    void rebuildTimelineItems(QListWidget* timelineList, CanvasWidget* canvas, MainWindow* window)
    {
        QSignalBlocker blocker(timelineList);

        timelineList->clear();

        for (int i = 0; i < canvas->getFrameCount(); ++i)
        {
            addFrameItem(timelineList, canvas, window, i);
        }

        addAddFrameItem(timelineList);

        int selectedIndex = canvas->getSelectedFrameIndex();

        if (selectedIndex >= 0 && selectedIndex < timelineList->count())
        {
            timelineList->setCurrentRow(selectedIndex);
        }
    }
}

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
    playAction->setCheckable(true);

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
    fpsBox->setObjectName("FpsSpinBox");
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
    onionSkinButton = new QPushButton("ONION SKIN: OFF");
    colorButton = new QPushButton("COLOUR");
    clearButton = new QPushButton("CLEAR CANVAS");
    clearButton->setObjectName("DangerButton");

    brushButton->setCheckable(true);
    eraserButton->setCheckable(true);
    moveButton->setCheckable(true);
    onionSkinButton->setCheckable(true);

    brushButton->setChecked(true);
    onionSkinButton->setChecked(false);

    QSpinBox* brushSizeBox = new QSpinBox();
    brushSizeBox->setRange(1, 50);
    brushSizeBox->setValue(6);

    QCheckBox* pressureCheckBox = new QCheckBox("Pressure Sensitivity");
    pressureCheckBox->setChecked(true);

    QSlider* pressureSlider = new QSlider(Qt::Horizontal);
    pressureSlider->setRange(0, 100);
    pressureSlider->setValue(70);

    pressureLabel = new QLabel("Pressure: 1.00");

    QSpinBox* onionSkinRangeBox = new QSpinBox();
    onionSkinRangeBox->setRange(1, 2);
    onionSkinRangeBox->setValue(1);

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
    toolLayout->addWidget(new QLabel("Onion Skin Frames"));
    toolLayout->addWidget(onionSkinRangeBox);
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

    connect(onionSkinRangeBox, &QSpinBox::valueChanged, this, [this](int value) {
        canvas->setOnionSkinRange(value);
        statusBar()->showMessage(
            "Onion skin range: " + QString::number(value) + " frame(s) back and forward"
        );
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
    timelineDock->setMinimumHeight(190);

    QWidget* timelineContainer = new QWidget();
    timelineContainer->setObjectName("TimelineContainer");

    QVBoxLayout* timelineLayout = new QVBoxLayout(timelineContainer);
    timelineLayout->setContentsMargins(12, 18, 12, 14);
    timelineLayout->setSpacing(0);

    timelineList = new QListWidget();
    timelineList->setObjectName("TimelineList");
    timelineList->setViewMode(QListView::IconMode);
    timelineList->setFlow(QListView::LeftToRight);
    timelineList->setWrapping(false);
    timelineList->setMovement(QListView::Static);
    timelineList->setResizeMode(QListView::Adjust);

    timelineList->setGridSize(QSize(TimelineCardWidth, TimelineCardHeight));
    timelineList->setSpacing(14);
    timelineList->setUniformItemSizes(true);

    timelineList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    timelineList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    timelineList->setMinimumHeight(TimelineCardHeight + 36);
    timelineList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    timelineList->setStyleSheet(R"(
        QListWidget#TimelineList {
            background-color: #171A21;
            border: 1px solid #2A3140;
            border-radius: 14px;
            padding: 12px;
            outline: none;
        }

        QListWidget#TimelineList::item {
            background: transparent;
            border: none;
            padding: 0px;
            margin: 0px;
        }

        QListWidget#TimelineList::item:hover {
            background: transparent;
            border: none;
        }

        QListWidget#TimelineList::item:selected {
            background: transparent;
            border: none;
        }

        QFrame#TimelineFrameCard {
            background-color: #0F1117;
            border: 1px solid #2A3140;
            border-radius: 14px;
        }

        QFrame#TimelineAddFrameCard {
            background-color: #0F1117;
            border: 1px dashed #2A3140;
            border-radius: 14px;
        }

        QPushButton#TimelineFrameSelectButton {
            background: transparent;
            color: #CBD5E1;
            border: none;
            padding: 0px;
            text-align: center;
            font-weight: 600;
        }

        QPushButton#TimelineFrameSelectButton:hover {
            color: #00D1FF;
        }

        QPushButton#TimelineAddButton {
            background: transparent;
            color: #CBD5E1;
            border: none;
            padding: 0px;
            text-align: center;
            font-weight: 600;
        }

        QPushButton#TimelineAddButton:hover {
            color: #00D1FF;
        }

        QPushButton#TimelineDeleteButton {
            background-color: #171A21;
            color: #FCA5A5;
            border: 1px solid #4B2430;
            border-radius: 12px;
            padding: 0px;
            margin: 0px;
            text-align: center;
            font-weight: 800;
        }

        QPushButton#TimelineDeleteButton:hover {
            background-color: #2A1118;
            color: #FB7185;
            border: 1px solid #FB7185;
        }
    )");

    timelineLayout->addWidget(timelineList);

    timelineDock->setWidget(timelineContainer);
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock);
    timelineDock->hide();

    rebuildTimelineItems(timelineList, canvas, this);
}

void MainWindow::connectActions()
{
    QTimer* playbackTimer = new QTimer(this);

    QSpinBox* fpsBox = findChild<QSpinBox*>("FpsSpinBox");

    if (fpsBox)
    {
        connect(fpsBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, playbackTimer](int fps) {
            if (playbackTimer->isActive())
            {
                playbackTimer->setInterval(playbackIntervalFromFps(fps));
                statusBar()->showMessage(
                    "Playback FPS changed to " + QString::number(fps)
                );
            }
            });
    }

    connect(playbackTimer, &QTimer::timeout, this, [this, playbackTimer, fpsBox]() {
        int frameCount = canvas->getFrameCount();

        if (frameCount <= 1)
        {
            playbackTimer->stop();

            QSignalBlocker blocker(playAction);
            playAction->setChecked(false);
            playAction->setText("Play");

            statusBar()->showMessage("Playback stopped: add more frames first");
            return;
        }

        if (fpsBox)
        {
            playbackTimer->setInterval(playbackIntervalFromFps(fpsBox->value()));
        }

        int currentFrame = canvas->getSelectedFrameIndex();
        int nextFrame = (currentFrame + 1) % frameCount;

        canvas->selectFrame(nextFrame);
        timelineList->setCurrentRow(nextFrame);

        statusBar()->showMessage(
            "Playing Frame " + QString::number(nextFrame + 1) +
            " / " + QString::number(frameCount)
        );
        });

    connect(playAction, &QAction::toggled, this, [this, playbackTimer, fpsBox](bool checked) {
        if (checked)
        {
            if (canvas->getFrameCount() <= 1)
            {
                QSignalBlocker blocker(playAction);
                playAction->setChecked(false);
                playAction->setText("Play");

                statusBar()->showMessage("Add at least 2 frames before playing");
                return;
            }

            int fps = fpsBox ? fpsBox->value() : 24;

            playAction->setText("Stop");
            playbackTimer->start(playbackIntervalFromFps(fps));

            statusBar()->showMessage(
                "Playback started at " + QString::number(fps) + " FPS"
            );
        }
        else
        {
            playbackTimer->stop();
            playAction->setText("Play");

            statusBar()->showMessage("Playback stopped");
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

    connect(onionSkinButton, &QPushButton::toggled, this, [this](bool checked) {
        canvas->setOnionSkinEnabled(checked);

        if (checked)
        {
            onionSkinButton->setText("ONION SKIN: ON");
            statusBar()->showMessage("Onion skin enabled: previous frames red, next frames green");
        }
        else
        {
            onionSkinButton->setText("ONION SKIN: OFF");
            statusBar()->showMessage("Onion skin disabled");
        }
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
        statusBar()->showMessage("Current frame cleared");
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

    connect(newAction, &QAction::triggered, this, [this, playbackTimer]() {
        playbackTimer->stop();

        QSignalBlocker blocker(playAction);
        playAction->setChecked(false);
        playAction->setText("Play");

        canvas->clearProject();
        canvas->resetZoom();
        canvas->resetRotation();

        rebuildTimelineItems(timelineList, canvas, this);

        statusBar()->showMessage("New project started");
        });

    connect(exitAction, &QAction::triggered, this, [this]() {
        close();
        });

    connect(timelineList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        QString itemType = item->data(ItemTypeRole).toString();

        if (itemType == "add")
        {
            canvas->addFrame();
            rebuildTimelineItems(timelineList, canvas, this);

            int frameNumber = canvas->getSelectedFrameIndex() + 1;

            statusBar()->showMessage("Frame " + QString::number(frameNumber) + " added");
            return;
        }

        if (itemType == "frame")
        {
            int frameIndex = item->data(FrameIndexRole).toInt();

            canvas->selectFrame(frameIndex);
            timelineList->setCurrentRow(frameIndex);

            statusBar()->showMessage("Frame " + QString::number(frameIndex + 1) + " selected");
        }
        });
}

void MainWindow::setActiveTool(QPushButton* activeButton)
{
    brushButton->setChecked(activeButton == brushButton);
    eraserButton->setChecked(activeButton == eraserButton);
    moveButton->setChecked(activeButton == moveButton);
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