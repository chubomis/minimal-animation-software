#pragma once

#include <QMainWindow>

class CanvasWidget;
class QAction;
class QDockWidget;
class QListWidget;
class QLabel;
class QPushButton;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildCentralCanvas();
    void buildShortcuts();
    void buildMenus();
    void buildTopToolbar();
    void buildToolDock();
    void buildTimelineDock();
    void connectActions();
    void setActiveTool(QPushButton* activeButton);
    void updateZoomStatus();
    void updateRotationStatus();

private:
    CanvasWidget* canvas = nullptr;

    QAction* newAction = nullptr;
    QAction* exitAction = nullptr;

    QAction* undoMenuAction = nullptr;
    QAction* redoMenuAction = nullptr;

    QAction* undoAction = nullptr;
    QAction* redoAction = nullptr;
    QAction* playAction = nullptr;

    QAction* timelineAction = nullptr;
    QAction* zoomInAction = nullptr;
    QAction* zoomOutAction = nullptr;
    QAction* resetZoomAction = nullptr;
    QAction* rotate90Action = nullptr;
    QAction* rotate180Action = nullptr;
    QAction* resetRotationAction = nullptr;

    QDockWidget* timelineDock = nullptr;
    QListWidget* timelineList = nullptr;

    QPushButton* brushButton = nullptr;
    QPushButton* eraserButton = nullptr;
    QPushButton* moveButton = nullptr;
    QPushButton* onionSkinButton = nullptr;
    QPushButton* colorButton = nullptr;
    QPushButton* clearButton = nullptr;

    QLabel* pressureLabel = nullptr;
};