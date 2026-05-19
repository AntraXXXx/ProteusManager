#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "databaseController.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_2_clicked()
{
    if (!m_databaseController)
    {
        m_databaseController = new Databasecontroller();
    }

    m_databaseController->show();
    m_databaseController->raise();
    m_databaseController->activateWindow();
}

