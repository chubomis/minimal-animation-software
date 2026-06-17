#include "MainWindow.h"

#include "CanvasWidget.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDockWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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
#include <QMessageBox>
#include <QProcess>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QTemporaryDir>
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

    QString withDefaultSuffix(QString filePath, const QString& suffix)
    {
        if (filePath.trimmed().isEmpty())
            return filePath;

        QFileInfo fileInfo(filePath);

        if (fileInfo.suffix().isEmpty())
        {
            filePath += suffix;
        }

        return filePath;
    }

    bool exportVideoWithFfmpeg(
        QWidget* parent,
        CanvasWidget* canvas,
        const QString& outputPath,
        int fps,
        QString* errorMessage
    )
    {
        QTemporaryDir temporaryDirectory;

        if (!temporaryDirectory.isValid())
        {
            if (errorMessage)
                *errorMessage = "Could not create a temporary folder for video export.";
            return false;
        }

        QString pngError;

        if (!canvas->exportFramesToPngSequence(temporaryDirectory.path(), &pngError))
        {
            if (errorMessage)
                *errorMessage = pngError;
            return false;
        }

        const QString inputPattern = QDir(temporaryDirectory.path()).filePath("frame_%04d.png");
        const QString suffix = QFileInfo(outputPath).suffix().toLower();

        QStringList arguments;
        arguments << "-y";
        arguments << "-framerate" << QString::number(std::clamp(fps, 1, 60));
        arguments << "-i" << inputPattern;

        if (suffix == "gif")
        {
            arguments << "-vf" << "split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse";
        }
        else
        {
            arguments << "-pix_fmt" << "yuv420p";
            arguments << "-vf" << "scale=trunc(iw/2)*2:trunc(ih/2)*2";
        }

        arguments << outputPath;

        QProcess ffmpeg(parent);
        ffmpeg.start("ffmpeg", arguments);

        if (!ffmpeg.waitForStarted(3000))
        {
            if (errorMessage)
            {
                *errorMessage =
                    "Could not start FFmpeg. Install FFmpeg and make sure ffmpeg.exe is in your PATH. "
                    "PNG frame export will still work without FFmpeg.";
            }

            return false;
        }

        ffmpeg.waitForFinished(-1);

        if (ffmpeg.exitStatus() != QProcess::NormalExit || ffmpeg.exitCode() != 0)
        {
            QString ffmpegOutput = QString::fromUtf8(ffmpeg.readAllStandardError()).trimmed();

            if (ffmpegOutput.isEmpty())
                ffmpegOutput = "FFmpeg failed, but did not return a detailed error.";

            if (errorMessage)
                *errorMessage = ffmpegOutput;

            return false;
        }

        return true;
    }

    QSize askStartupCanvasSize(QWidget* parent)
    {
        constexpr int DefaultWidth = 1280;
        constexpr int DefaultHeight = 720;

        QDialog dialog(parent);
        dialog.setWindowTitle("New Canvas Size");
        dialog.setModal(true);

        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(12);

        QLabel* titleLabel = new QLabel("Choose your canvas size");
        QFont titleFont = titleLabel->font();
        titleFont.setBold(true);
        titleFont.setPointSize(12);
        titleLabel->setFont(titleFont);

        QLabel* hintLabel = new QLabel("This controls the actual drawing area for the project.");
        hintLabel->setWordWrap(true);

        QSpinBox* widthBox = new QSpinBox();
        widthBox->setRange(100, 8192);
        widthBox->setValue(DefaultWidth);
        widthBox->setSuffix(" px");

        QSpinBox* heightBox = new QSpinBox();
        heightBox->setRange(100, 8192);
        heightBox->setValue(DefaultHeight);
        heightBox->setSuffix(" px");

        QFormLayout* formLayout = new QFormLayout();
        formLayout->setContentsMargins(0, 0, 0, 0);
        formLayout->setSpacing(10);
        formLayout->addRow("Width", widthBox);
        formLayout->addRow("Height", heightBox);

        QHBoxLayout* presetLayout = new QHBoxLayout();
        presetLayout->setContentsMargins(0, 0, 0, 0);
        presetLayout->setSpacing(8);

        QPushButton* hdPresetButton = new QPushButton("1280 x 720");
        QPushButton* fullHdPresetButton = new QPushButton("1920 x 1080");
        QPushButton* squarePresetButton = new QPushButton("1080 x 1080");

        presetLayout->addWidget(hdPresetButton);
        presetLayout->addWidget(fullHdPresetButton);
        presetLayout->addWidget(squarePresetButton);

        QObject::connect(hdPresetButton, &QPushButton::clicked, &dialog, [widthBox, heightBox]() {
            widthBox->setValue(1280);
            heightBox->setValue(720);
            });

        QObject::connect(fullHdPresetButton, &QPushButton::clicked, &dialog, [widthBox, heightBox]() {
            widthBox->setValue(1920);
            heightBox->setValue(1080);
            });

        QObject::connect(squarePresetButton, &QPushButton::clicked, &dialog, [widthBox, heightBox]() {
            widthBox->setValue(1080);
            heightBox->setValue(1080);
            });

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setText("Create Canvas");
        buttons->button(QDialogButtonBox::Cancel)->setText("Use Default");

        QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        layout->addWidget(titleLabel);
        layout->addWidget(hintLabel);
        layout->addLayout(formLayout);
        layout->addWidget(new QLabel("Presets"));
        layout->addLayout(presetLayout);
        layout->addWidget(buttons);

        if (dialog.exec() == QDialog::Accepted)
        {
            return QSize(widthBox->value(), heightBox->value());
        }

        return QSize(DefaultWidth, DefaultHeight);
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

    const QSize canvasSize = askStartupCanvasSize(this);

    buildCentralCanvas(canvasSize);
    buildShortcuts();
    buildMenus();
    buildTopToolbar();
    buildToolDock();
    buildTimelineDock();
    connectActions();

    statusBar()->showMessage(
        "Ready - Canvas " + QString::number(canvas->getCanvasSize().width()) +
        " x " + QString::number(canvas->getCanvasSize().height())
    );
}

void MainWindow::buildCentralCanvas(const QSize& canvasSize)
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
    canvas->setCanvasSize(canvasSize);

    canvasFrameLayout->addWidget(canvas);
    centerLayout->addWidget(canvasFrame);

    setCentralWidget(centerArea);
}

void MainWindow::buildShortcuts()
{
    QShortcut* zoomInShortcut = new QShortcut(QKeySequence::ZoomIn, this);
    QShortcut* zoomInShortcutAlt = new QShortcut(QKeySequence("Ctrl+="), this);
    QShortcut* zoomOutShortcut = new QShortcut(QKeySequence::ZoomOut, this);
    QShortcut* centerCanvasShortcut = new QShortcut(QKeySequence("Ctrl+0"), this);

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

    connect(centerCanvasShortcut, &QShortcut::activated, this, [this]() {
        canvas->centerCanvas();
        statusBar()->showMessage("Canvas centered");
        });
}

void MainWindow::buildMenus()
{
    QMenu* fileMenu = menuBar()->addMenu("File");

    newAction = fileMenu->addAction("New");
    newAction->setShortcut(QKeySequence("Ctrl+N"));
    newAction->setShortcutContext(Qt::WindowShortcut);

    saveAction = fileMenu->addAction("Save Animation");
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    saveAction->setShortcutContext(Qt::WindowShortcut);

    importAction = fileMenu->addAction("Import Animation");
    importAction->setShortcut(QKeySequence("Ctrl+O"));
    importAction->setShortcutContext(Qt::WindowShortcut);

    fileMenu->addSeparator();

    exportPngSequenceAction = fileMenu->addAction("Export PNG Frames");
    exportVideoAction = fileMenu->addAction("Export Video");
    exportVideoAction->setShortcut(QKeySequence("Ctrl+E"));
    exportVideoAction->setShortcutContext(Qt::WindowShortcut);

    fileMenu->addSeparator();

    exitAction = fileMenu->addAction("Exit");

    QMenu* editMenu = menuBar()->addMenu("Edit");

    undoMenuAction = editMenu->addAction("Undo");
    undoMenuAction->setShortcut(QKeySequence("Ctrl+Z"));
    undoMenuAction->setShortcutContext(Qt::WindowShortcut);

    redoMenuAction = editMenu->addAction("Redo");
    redoMenuAction->setShortcut(QKeySequence("Ctrl+Y"));
    redoMenuAction->setShortcutContext(Qt::WindowShortcut);

    editMenu->addSeparator();

    copyFrameAction = editMenu->addAction("Copy Frame");
    copyFrameAction->setShortcut(QKeySequence("Ctrl+C"));
    copyFrameAction->setShortcutContext(Qt::WindowShortcut);

    pasteFrameAction = editMenu->addAction("Paste Frame");
    pasteFrameAction->setShortcut(QKeySequence("Ctrl+V"));
    pasteFrameAction->setShortcutContext(Qt::WindowShortcut);

    addAction(newAction);
    addAction(saveAction);
    addAction(importAction);
    addAction(exportVideoAction);
    addAction(undoMenuAction);
    addAction(redoMenuAction);
    addAction(copyFrameAction);
    addAction(pasteFrameAction);

    menuBar()->addMenu("View");
}

void MainWindow::buildTopToolbar()
{
    QToolBar* topToolbar = addToolBar("Top Controls");
    topToolbar->setMovable(false);

    topToolbar->addAction(saveAction);
    topToolbar->addAction(importAction);
    topToolbar->addAction(exportVideoAction);

    topToolbar->addSeparator();

    undoAction = topToolbar->addAction("Undo");
    redoAction = topToolbar->addAction("Redo");

    topToolbar->addSeparator();
    topToolbar->addAction(copyFrameAction);
    topToolbar->addAction(pasteFrameAction);

    topToolbar->addSeparator();

    playAction = topToolbar->addAction("Play");
    playAction->setCheckable(true);

    timelineAction = topToolbar->addAction("Timeline");
    timelineAction->setCheckable(true);
    timelineAction->setChecked(false);

    menuBar()->actions().last()->menu()->addAction(timelineAction);

    zoomInAction = topToolbar->addAction("Zoom +");
    zoomOutAction = topToolbar->addAction("Zoom -");
    centerCanvasAction = topToolbar->addAction("Center");

    rotate90Action = topToolbar->addAction("Rot 90");
    rotate180Action = topToolbar->addAction("Rot 180");
    resetRotationAction = topToolbar->addAction("Reset Rot");

    topToolbar->addSeparator();
    topToolbar->addWidget(new QLabel("FPS"));

    fpsBox = new QSpinBox();
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
    onionSkinButton = new QPushButton("ONION SKIN: OFF");
    colorButton = new QPushButton("COLOUR");
    clearButton = new QPushButton("CLEAR CANVAS");
    clearButton->setObjectName("DangerButton");

    brushButton->setCheckable(true);
    eraserButton->setCheckable(true);
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

    connect(playbackTimer, &QTimer::timeout, this, [this, playbackTimer]() {
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

    connect(playAction, &QAction::toggled, this, [this, playbackTimer](bool checked) {
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

    connect(centerCanvasAction, &QAction::triggered, this, [this]() {
        canvas->centerCanvas();
        statusBar()->showMessage("Canvas centered");
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

    connect(copyFrameAction, &QAction::triggered, this, [this]() {
        const int frameIndex = canvas->getSelectedFrameIndex();

        if (canvas->copyFrame(frameIndex))
        {
            statusBar()->showMessage("Frame " + QString::number(frameIndex + 1) + " copied");
        }
        else
        {
            statusBar()->showMessage("Could not copy frame");
        }
        });

    connect(pasteFrameAction, &QAction::triggered, this, [this]() {
        const int frameIndex = canvas->getSelectedFrameIndex();

        if (!canvas->hasCopiedFrame())
        {
            statusBar()->showMessage("Copy a frame before pasting");
            return;
        }

        if (canvas->pasteFrameToCurrent())
        {
            rebuildTimelineItems(timelineList, canvas, this);
            timelineList->setCurrentRow(frameIndex);

            statusBar()->showMessage("Copied drawing pasted into Frame " + QString::number(frameIndex + 1));
        }
        else
        {
            statusBar()->showMessage("Could not paste frame");
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

        const QSize newCanvasSize = askStartupCanvasSize(this);

        canvas->clearProject();
        canvas->setCanvasSize(newCanvasSize);
        canvas->resetZoom();
        canvas->resetRotation();
        canvas->centerCanvas();

        rebuildTimelineItems(timelineList, canvas, this);

        statusBar()->showMessage(
            "New project started - Canvas " +
            QString::number(newCanvasSize.width()) + " x " +
            QString::number(newCanvasSize.height())
        );
        });

    connect(saveAction, &QAction::triggered, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this,
            "Save Animation",
            "untitled.minanim",
            "Minimal Animation Project (*.minanim);;JSON Project (*.json)"
        );

        if (filePath.isEmpty())
            return;

        filePath = withDefaultSuffix(filePath, ".minanim");

        QString errorMessage;
        const int fps = fpsBox ? fpsBox->value() : 24;

        if (!canvas->saveProjectToFile(filePath, fps, &errorMessage))
        {
            QMessageBox::warning(this, "Save Failed", errorMessage);
            return;
        }

        statusBar()->showMessage("Animation saved: " + QFileInfo(filePath).fileName());
        });

    connect(importAction, &QAction::triggered, this, [this, playbackTimer]() {
        playbackTimer->stop();

        QSignalBlocker blocker(playAction);
        playAction->setChecked(false);
        playAction->setText("Play");

        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Import Animation",
            QString(),
            "Minimal Animation Project (*.minanim *.json);;All Files (*.*)"
        );

        if (filePath.isEmpty())
            return;

        QString errorMessage;
        int importedFps = 24;

        if (!canvas->loadProjectFromFile(filePath, &importedFps, &errorMessage))
        {
            QMessageBox::warning(this, "Import Failed", errorMessage);
            return;
        }

        if (fpsBox)
        {
            fpsBox->setValue(importedFps);
        }

        rebuildTimelineItems(timelineList, canvas, this);
        timelineList->setCurrentRow(canvas->getSelectedFrameIndex());

        statusBar()->showMessage("Animation imported: " + QFileInfo(filePath).fileName());
        });

    connect(exportPngSequenceAction, &QAction::triggered, this, [this]() {
        QString directoryPath = QFileDialog::getExistingDirectory(
            this,
            "Choose Folder for PNG Frames"
        );

        if (directoryPath.isEmpty())
            return;

        QString errorMessage;

        if (!canvas->exportFramesToPngSequence(directoryPath, &errorMessage))
        {
            QMessageBox::warning(this, "PNG Export Failed", errorMessage);
            return;
        }

        statusBar()->showMessage(
            "Exported " + QString::number(canvas->getFrameCount()) + " PNG frame(s)"
        );
        });

    connect(exportVideoAction, &QAction::triggered, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this,
            "Export Video",
            "animation.mp4",
            "MP4 Video (*.mp4);;GIF Animation (*.gif)"
        );

        if (filePath.isEmpty())
            return;

        QFileInfo selectedFileInfo(filePath);

        if (selectedFileInfo.suffix().isEmpty())
        {
            filePath += ".mp4";
        }

        QString errorMessage;
        const int fps = fpsBox ? fpsBox->value() : 24;

        if (!exportVideoWithFfmpeg(this, canvas, filePath, fps, &errorMessage))
        {
            QMessageBox::warning(this, "Video Export Failed", errorMessage);
            return;
        }

        statusBar()->showMessage("Video exported: " + QFileInfo(filePath).fileName());
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