#include <QApplication>
#include <QFont>

#include "AppStyle.h"
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QApplication::setStyle("Fusion");
    app.setFont(QFont("Segoe UI", 9));
    app.setStyleSheet(neonStyleSheet());

    MainWindow window;
    window.show();

    return app.exec();
}