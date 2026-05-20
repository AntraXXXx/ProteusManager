#include "windows/mainwindow.h"
#include <QApplication>



int main(int argc, char *argv[])
{
    //datareader.readTextFile("example.txt");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return QCoreApplication::exec();
}


