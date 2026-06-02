#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "databasecontroller.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_ollamaClient(new OllamaClient(this))
    , m_dataBaseManager(new DatabaseManager())
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
                    ui->pushButton_ConnectDB->setEnabled(false);

                    return;
                }

                ui->pushButton_SqlGenerator->setEnabled(false);

                for (const QString& model : models)
                {
                    ui->comboBox_AiModell->addItem(model);
                }

               m_selectedModel = ui->comboBox_AiModell->currentText();
            });


    m_ollamaClient->fetchModels();


    QSettings settings("DataBaseSettings","Proteus");

    bool isLocal = settings.value("database/isLocalConnection",true).toBool();
    ui->checkBox_isLocalDatabase->setChecked(isLocal);

    ui->label_localDatabase->setHidden(!isLocal);
    ui->lineEdit_localdatabasepath->setHidden(!isLocal);
    ui->pushButton_adddatabasedir->setHidden(!isLocal);

    // Online database
    ui->label_DataBaseAddress->setHidden(isLocal);
    ui->lineEdit_DataBaseAddress->setHidden(isLocal);
    ui->label_HostName->setHidden(isLocal);
    ui->lineEdit_HostName->setHidden(isLocal);
    ui->label_Password->setHidden(isLocal);
    ui->lineEdit_Password->setHidden(isLocal);

    ui->pushButton_SqlGenerator->setEnabled(false);
    ui->pushButton_ConnectDB->setStyleSheet("background-color: red;");


    QList<ProgrammingLanguage::ProgrammingLanguageType> languages =
        {
            ProgrammingLanguage::ProgrammingLanguageType::Cplusplus,
            ProgrammingLanguage::ProgrammingLanguageType::C,
            ProgrammingLanguage::ProgrammingLanguageType::Csharp,
            ProgrammingLanguage::ProgrammingLanguageType::Python,
            ProgrammingLanguage::ProgrammingLanguageType::Go,
            ProgrammingLanguage::ProgrammingLanguageType::Rust,
            ProgrammingLanguage::ProgrammingLanguageType::Fsharp,
            ProgrammingLanguage::ProgrammingLanguageType::Powershell,
            ProgrammingLanguage::ProgrammingLanguageType::Java
        };


    for (auto language : languages)
    {
        ui->comboBox_CodeLanguage->addItem(
            ProgrammingLanguage::languageTypeToText(language),
            static_cast<int>(language)
            );
    }
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
          m_tableGenerator = new Tablegenerator(m_dataBaseManager);

        connect(
            m_tableGenerator,
            &Tablegenerator::windowClosed,
            this,
            [this]()
            {
                ui->pushButton_ConnectDB->setEnabled(true);
                ui->pushButton_ConnectDB->setStyleSheet("background-color: none;");
                ui->pushButton_ConnectDB->setText("Connect");
                ui->pushButton_SqlGenerator->setEnabled(false);
            });
    }

    m_tableGenerator->setSelectedModel(m_selectedModel);
    m_tableGenerator->setSelectedLanguage(m_selectedLanguageType);

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

void MainWindow::on_checkBox_isLocalDatabase_checkStateChanged(const Qt::CheckState &arg1)
{
    ui->label_localDatabase->setHidden(!ui->checkBox_isLocalDatabase->isChecked());
    ui->lineEdit_localdatabasepath->setHidden(!ui->checkBox_isLocalDatabase->isChecked());
    ui->pushButton_adddatabasedir->setHidden(!ui->checkBox_isLocalDatabase->isChecked());

    // Online database
    ui->label_DataBaseAddress->setHidden(ui->checkBox_isLocalDatabase->isChecked());
    ui->lineEdit_DataBaseAddress->setHidden(ui->checkBox_isLocalDatabase->isChecked());
    ui->label_HostName->setHidden(ui->checkBox_isLocalDatabase->isChecked());
    ui->lineEdit_HostName->setHidden(ui->checkBox_isLocalDatabase->isChecked());
    ui->label_Password->setHidden(ui->checkBox_isLocalDatabase->isChecked());
    ui->lineEdit_Password->setHidden(ui->checkBox_isLocalDatabase->isChecked());
}

void MainWindow::on_pushButton_adddatabasedir_clicked()
{
    QString databasePath =
        QFileDialog::getOpenFileName(
            this,
            "Select a database",
            "",
            "Database (*.db *.sqlite)"
            );

    if (databasePath.isEmpty())
        return;

    m_dataBaseManager->setDatabasePath(databasePath);

    ui->lineEdit_localdatabasepath->setText(databasePath);
}

void MainWindow::on_pushButton_ConnectDB_clicked()
{


    QFileInfo fileInfo(ui->lineEdit_localdatabasepath->text());

    QString connectionName = fileInfo.fileName();
    QString filePath =  ui->lineEdit_localdatabasepath->text();

//   bool success =
//       m_dataBaseManager->openDatabase(
//           connectionName,
//          filePath
//          );

    if(!connectionName.isEmpty() && !filePath.isEmpty())
    {
        m_dataBaseManager->openDatabase(connectionName, filePath);

        if(m_dataBaseManager->isConnected())
        {
            ui->pushButton_ConnectDB->setStyleSheet("background-color: green;");
            ui->pushButton_ConnectDB->setText("Connected");
            ui->pushButton_ConnectDB->setEnabled(false);
            ui->pushButton_SqlGenerator->setEnabled(true);
        }
        else
        {
            ui->pushButton_ConnectDB->setEnabled(true);
            ui->pushButton_ConnectDB->setStyleSheet("background-color: red;");
            ui->pushButton_SqlGenerator->setEnabled(false);
            return;
        }
    }
}

void MainWindow::on_comboBox_CodeLanguage_currentIndexChanged(int index)
{
    m_selectedLanguageType =
        static_cast<ProgrammingLanguage::ProgrammingLanguageType>(
            ui->comboBox_CodeLanguage->itemData(index).toInt()
            );

    if (m_tableGenerator)
    {
        m_tableGenerator->setSelectedLanguage(m_selectedLanguageType);
    }
}


void MainWindow::on_pushButton_Exit_clicked()
{
    QApplication::quit();
}

