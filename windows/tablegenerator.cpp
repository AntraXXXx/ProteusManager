#include "tablegenerator.h"
#include "ui_tablegenerator.h"
//#include "../utils/textfilemanager.h"
#include "../utils/classscanner.h"
#include "../utils/classparser.h"

Tablegenerator::Tablegenerator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tablegenerator)
    , m_ollamaClient(new OllamaClient(this))
{
    ui->setupUi(this);
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
    ClassScanner scanner;
    ClassParser parser;

    QList<ScannedClassFile> files =
        scanner.scanAndReadClassFiles(m_classPath);

    QString prompt =
        "Generate SQLite CREATE TABLE statements from these classes. "
        "Return only valid SQL, no explanation. Please only a valid SQL\n\n";

    for (const ScannedClassFile& file : files)
    {
        QList<ParsedClass> classes =
            parser.parseCppClasses(file.content);

        for (const ParsedClass& cls : classes)
        {
            prompt += "Class: " + cls.name + "\n";

            for (const ParsedAttribute& attribute : cls.attributes)
            {
                prompt += "- "
                          + attribute.type
                          + " "
                          + attribute.name;

                if (attribute.isRelation)
                    prompt += " relationship";

                prompt += "\n";
            }

            prompt += "\n";
        }
    }

    qDebug() << "Prompt:" << prompt;

    m_ollamaClient->generateSql(m_selectedModel, prompt);
}
