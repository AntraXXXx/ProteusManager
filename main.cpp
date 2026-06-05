//#include "windows/mainwindow.h"
//#include <QApplication>



// int main(int argc, char *argv[])
// {
//     //datareader.readTextFile("example.txt");

//     QApplication a(argc, argv);
//     MainWindow w;
//     w.show();

//     return QCoreApplication::exec();
// }


#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "appcontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Fusion");

    AppController appController;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        "appController",
        &appController
        );

    engine.loadFromModule("DataBaseManager", "Main");

    return app.exec();
}