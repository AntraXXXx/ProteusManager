#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "databasecontroller.h"
#include "tablegenerator.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_ollamaClient(new OllamaClient(this))
{
    ui->setupUi(this);

    connect(m_ollamaClient,
            &OllamaClient::modelsFetched,
            this,
            [this](const QStringList& models)
            {
                ui->comboBox_AiModell->clear();

                if (models.isEmpty())
                {
                    ui->comboBox_AiModell
                        ->addItem("No model is installed on the device");

                    ui->pushButton_SqlGenerator->setEnabled(false);

                    return;
                }

                ui->pushButton_SqlGenerator->setEnabled(true);

                for (const QString& model : models)
                {
                    ui->comboBox_AiModell->addItem(model);
                }

               m_selectedModel = ui->comboBox_AiModell->currentText();
            });


    m_ollamaClient->fetchModels();
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

    m_tableGenerator->setSelectedModel(m_selectedModel);

    m_tableGenerator->show();
    m_tableGenerator->raise();
    m_tableGenerator->activateWindow();
}

void MainWindow::on_comboBox_AiModell_currentIndexChanged(int index)
{
    m_selectedModel = ui->comboBox_AiModell->itemText(index);

    if (m_tableGenerator)
    {
        m_tableGenerator->setSelectedModel(m_selectedModel);
    }
}
