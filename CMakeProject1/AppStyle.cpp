#include "AppStyle.h"

QString neonStyleSheet()
{
    return R"(
        QMainWindow {
            background-color: #0F1117;
        }

        QWidget#CenterArea {
            background-color: #0F1117;
        }

        QFrame#CanvasFrame {
            background-color: #171A21;
            border: 1px solid #2A3140;
            border-radius: 18px;
            padding: 10px;
        }

        QMenuBar {
            background-color: #0F1117;
            color: #E5E7EB;
            border-bottom: 1px solid #252A36;
            padding: 4px;
        }

        QMenuBar::item {
            background: transparent;
            padding: 6px 12px;
            border-radius: 6px;
        }

        QMenuBar::item:selected {
            background-color: #171A21;
            color: #00D1FF;
        }

        QMenu {
            background-color: #171A21;
            color: #E5E7EB;
            border: 1px solid #2A3140;
            padding: 6px;
        }

        QMenu::item {
            padding: 6px 24px;
            border-radius: 5px;
        }

        QMenu::item:selected {
            background-color: #102633;
            color: #00D1FF;
        }

        QToolBar {
            background-color: #171A21;
            border: none;
            border-bottom: 1px solid #252A36;
            spacing: 8px;
            padding: 8px;
        }

        QToolButton {
            background-color: #0F1117;
            color: #CBD5E1;
            border: 1px solid #2A3140;
            border-radius: 10px;
            padding: 7px 12px;
            font-weight: 600;
        }

        QToolButton:hover {
            border: 1px solid #00D1FF;
            color: #00D1FF;
            background-color: #102633;
        }

        QToolButton:checked {
            border: 2px solid #00D1FF;
            color: #FAFAFA;
            background-color: #102633;
        }

        QDockWidget {
            background-color: #171A21;
            color: #E5E7EB;
        }

        QDockWidget::title {
            background-color: #171A21;
            color: #00D1FF;
            padding: 9px;
            border-bottom: 1px solid #252A36;
            font-weight: 700;
            text-align: left;
        }

        QWidget {
            background-color: #171A21;
            color: #E5E7EB;
            font-size: 13px;
        }

        QLabel {
            color: #AAB2C0;
            background: transparent;
        }

        QPushButton {
            background-color: #0F1117;
            color: #CBD5E1;
            border: 1px solid #2A3140;
            border-radius: 10px;
            padding: 9px 12px;
            text-align: left;
            font-weight: 600;
        }

        QPushButton:hover {
            border: 1px solid #00D1FF;
            color: #00D1FF;
            background-color: #102633;
        }

        QPushButton:checked {
            border: 2px solid #00D1FF;
            color: #FAFAFA;
            background-color: #102633;
        }

        QPushButton#DangerButton {
            border: 1px solid #4B2430;
            color: #FCA5A5;
        }

        QPushButton#DangerButton:hover {
            border: 1px solid #FB7185;
            color: #FB7185;
            background-color: #2A1118;
        }

        QSpinBox {
            background-color: #0F1117;
            color: #FAFAFA;
            border: 1px solid #2A3140;
            border-radius: 8px;
            padding: 6px;
        }

        QSpinBox:focus {
            border: 1px solid #00D1FF;
        }

        QCheckBox {
            color: #CBD5E1;
            spacing: 8px;
            background: transparent;
        }

        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 5px;
            border: 1px solid #2A3140;
            background-color: #0F1117;
        }

        QCheckBox::indicator:checked {
            background-color: #00D1FF;
            border: 1px solid #00D1FF;
        }

        QSlider {
            background: transparent;
        }

        QSlider::groove:horizontal {
            height: 6px;
            background: #2A3140;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: #00D1FF;
            width: 16px;
            height: 16px;
            margin: -5px 0;
            border-radius: 8px;
        }

        QSlider::sub-page:horizontal {
            background: #00D1FF;
            border-radius: 3px;
        }

        QListWidget#TimelineList {
            background-color: #171A21;
            border: 1px solid #2A3140;
            border-radius: 14px;
            padding: 8px;
            outline: none;
        }

        QListWidget#TimelineList::item {
            background-color: #0F1117;
            color: #CBD5E1;
            border: 1px solid #2A3140;
            border-radius: 12px;
            padding: 16px;
            margin: 6px;
        }

        QListWidget#TimelineList::item:hover {
            border: 1px solid #A78BFA;
            color: #FAFAFA;
        }

        QListWidget#TimelineList::item:selected {
            background-color: #102633;
            color: #FAFAFA;
            border: 2px solid #00D1FF;
        }

        QStatusBar {
            background-color: #0B0D12;
            color: #A78BFA;
            border-top: 1px solid #252A36;
        }
    )";
}