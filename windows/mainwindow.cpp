#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "databasecontroller.h"
#include "tablegenerator.h"

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

void MainWindow::on_pushButton_SqlGenerator_clicked()
{
    // if (!m_databaseController)
    //  {
    //      m_databaseController = new DatabaseController();
    //  }

    //  m_databaseController->show();
    //  m_databaseController->raise();
    //  m_databaseController->activateWindow();

    if (!m_tableGenerator)
    {
        m_tableGenerator = new Tablegenerator();
    }

    m_tableGenerator->show();
    m_tableGenerator->raise();
    m_tableGenerator->activateWindow();
}