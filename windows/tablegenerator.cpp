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

    ClassScanner scanner;
    ClassParser parser;

    QList<ScannedClassFile> files =
        scanner.scanAndReadClassFiles(m_classPath);

    m_prompt =
        "You are a professional SQLite database architect. "
        "Analyze all provided classes and attributes. "
        "Include every detected class as a table and every detected attribute as a column. "
        "Do not omit, simplify, or ignore any class, attribute, datatype, or relationship. "
        "Generate a complete and normalized SQLite database schema. "
        "Create primary keys and foreign keys where relationships are detected. "
        "Use appropriate SQLite datatypes. "
        "Return only executable CREATE TABLE statements. "
        "No markdown. No comments. No explanation.\n\n";

    for (const ScannedClassFile& file : files)
    {
        QList<ParsedClass> classes =
            parser.parseCppClasses(file.content);

        for (const ParsedClass& cls : classes)
        {
            m_prompt += "Class: " + cls.name + "\n";

            for (const ParsedAttribute& attribute : cls.attributes)
            {
                m_prompt += "- "
                          + attribute.type
                          + " "
                          + attribute.name;

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

