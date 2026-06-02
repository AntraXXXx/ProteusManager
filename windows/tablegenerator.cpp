#include "tablegenerator.h"
#include "ui_tablegenerator.h"
//#include "../utils/textfilemanager.h"
#include "../utils/classscanner.h"
#include "../utils/classparser.h"

Tablegenerator::Tablegenerator(DatabaseManager *databaseManager,
                               QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tablegenerator)
    , m_ollamaClient(new OllamaClient(this))
    , m_dataBaseManager(databaseManager)
{
    ui->setupUi(this);
    ui->progressBar_loading->hide();
    ui->pushButton_execute->setEnabled(false);

    connect(
        m_ollamaClient,
        &OllamaClient::responseReceived,
        this,
        [this](const QString& response)
        {
           // ui->progressBar_loading->hide();
            ui->pushButton_generate->setEnabled(true);

            ui->plainTextEdit_ai->setPlainText(response);

            if(!response.isEmpty()){
            ui->pushButton_execute->setEnabled(true);
            }

        });

    connect(
        m_ollamaClient,
        &OllamaClient::errorOccurred,
        this,
        [this](const QString& error)
        {
            ui->progressBar_loading->hide();
            ui->pushButton_generate->setEnabled(true);

            QMessageBox::critical(this, "Ollama Error", error);
        });
}

Tablegenerator::~Tablegenerator()
{
    delete ui;
}

void Tablegenerator::setSelectedModel(const QString& model)
{
    m_selectedModel = model;
}

void Tablegenerator::setSelectedLanguage(
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    m_selectedLanguage = language;
}

void Tablegenerator::on_pushButton_addclasses_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select a folder");

    if (!folderPath.isEmpty()) {
        ui->lineEdit_classespath->setText(folderPath);
        m_classPath = folderPath;
    }
}

void Tablegenerator::on_pushButton_generate_clicked()
{
    ui->progressBar_loading->setRange(0, 0);
    ui->progressBar_loading->show();
    ui->pushButton_generate->setEnabled(false);


    if (m_classPath.isEmpty())
    {
        QMessageBox::warning(this, "Missing path", "Please select a class folder first.");
        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    if (m_selectedModel.isEmpty())
    {
        QMessageBox::warning(this, "Missing AI model", "Please select an AI model first.");
        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    if (!m_dataBaseManager->isConnected())
    {
        QMessageBox::warning(
            this,
            "Database",
            "Please connect to a database first."
            );

        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    ClassScanner scanner;
    ClassParser parser;


    QList<ScannedClassFile> files =scanner.scanAndReadClassFiles(m_classPath, m_selectedLanguage);

    if (files.isEmpty())
    {
        QMessageBox::warning(
            this,
            "No files found",
            "No class files were found in the selected folder."
            );

        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    m_prompt =
        "You are a professional SQLite database architect. "
        "Analyze all provided classes, attributes and database metadata. "

        "Use only the provided class names, attribute names and database information. "
        "Never rename classes. "
        "Never rename attributes. "
        "Never replace an existing attribute with a different name. "
        "Attribute names must remain exactly as provided. "
        "If an attribute is named username, it must remain username. "
        "Do not convert username to name or any other synonym. "
        "Do not invent attributes that are not provided. "
        "Respect exact attribute names, spelling and casing. "

        "If a table does not exist, generate a CREATE TABLE statement. "
        "If a table already exists, do not recreate it. "
        "If an attribute is missing in an existing table, generate ALTER TABLE ADD COLUMN. "
        "If an attribute already exists, do not generate it again. "

        "Never drop, delete, recreate or overwrite existing tables. "
        "Never remove existing columns. "
        "Never destroy existing data. "

        "Every detected class and attribute must be considered exactly as provided. "
        "Use appropriate SQLite datatypes. "
        "Create primary keys and foreign keys where appropriate. "
        "Use AUTOINCREMENT only when automatic identifier generation is logically required. ";

    if (ui->checkBox_aduitfields->isChecked())
    {
        m_prompt +=
            "Add createdAt and updatedAt columns to all generated tables. ";
    }
    else
    {
        m_prompt +=
            "Do not add createdAt or updatedAt unless they are explicitly defined in the provided classes. ";
    }

    m_prompt +=
        "Return only executable SQLite SQL statements. "
        "No markdown. "
        "No comments. "
        "No explanation.\n\n";

    for (const ScannedClassFile& file : files)
    {
        QList<ParsedClass> classes =
            parser.parseClasses(
                file.content,
                m_selectedLanguage
                );

        for (const ParsedClass& cls : classes)
        {
            m_prompt += "Class: " + cls.name + "\n";

            if (m_dataBaseManager->tableExists(cls.name))
            {
                m_prompt += "Existing table: yes\n";

                if (m_dataBaseManager->hasRows(cls.name))
                    m_prompt += "Table contains data: yes\n";
                else
                    m_prompt += "Table contains data: no\n";

                QStringList existingColumns =
                    m_dataBaseManager->getColumnNames(cls.name);

                m_prompt += "Existing columns: "
                            + existingColumns.join(", ")
                            + "\n";
            }
            else
            {
                m_prompt += "Existing table: no\n";
            }

            for (const ParsedAttribute& attribute : cls.attributes)
            {
                m_prompt += "- "
                            + attribute.type
                            + " "
                            + attribute.name;

                if (m_dataBaseManager->tableExists(cls.name))
                {
                    if (m_dataBaseManager->columnExists(cls.name, attribute.name))
                        m_prompt += " existing_column";
                    else
                        m_prompt += " missing_column";
                }

                if (attribute.isRelation)
                    m_prompt += " relationship";

                m_prompt += "\n";
            }

            m_prompt += "\n";
        }
    }

    qDebug() << "Prompt:" << m_prompt;
    m_ollamaClient->generateSql(m_selectedModel, m_prompt);
}

void Tablegenerator::on_pushButton_execute_clicked()
{
    QString response = ui->plainTextEdit_ai->toPlainText();

    if (!m_dataBaseManager->isValidSql(response))
    {
        QMessageBox::warning(
            this,
            "Invalid SQL",
            "The AI response does not contain valid SQL."
            );
        ui->progressBar_loading->hide();

        return;
    }

    if (m_dataBaseManager->executeQuery(response))
    {
        QMessageBox::information(
            this,
            "Database",
            "SQL executed successfully."
            );
        ui->progressBar_loading->hide();
        ui->pushButton_execute->setEnabled(false);
    }
}

void Tablegenerator::on_pushButton_back_clicked()
{
    this->close();
}

void Tablegenerator::closeEvent(QCloseEvent *event)
{
    emit windowClosed();

    QWidget::closeEvent(event);
}
