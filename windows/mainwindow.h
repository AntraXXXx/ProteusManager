#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "databasecontroller.h"
#include "tablegenerator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_pushButton_SqlGenerator_clicked();

private:
    Ui::MainWindow *ui;
    DatabaseController *m_databaseController = nullptr;
    Tablegenerator *m_tableGenerator = nullptr;
};
#endif
