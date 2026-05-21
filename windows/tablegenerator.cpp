#include "tablegenerator.h"
#include "ui_tablegenerator.h"
//#include "../utils/textfilemanager.h"
#include "../utils/classscanner.h"
#include "../utils/classparser.h"

Tablegenerator::Tablegenerator(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tablegenerator)
{
    ui->setupUi(this);
}

Tablegenerator::~Tablegenerator()
{
    delete ui;
}

void Tablegenerator::on_pushButton_addclasses_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select a folder");

    if (!folderPath.isEmpty()) {
        ui->lineEdit_classespath->setText(folderPath);
        m_classPath = folderPath;
    }
}

void Tablegenerator::on_pushButton_adddatabasedir_clicked()
{
    m_databasePath =
        QFileDialog::getOpenFileName(
            this,
            "Select a database",
            "",
             "database (*.db *.sqlite)"
            );

    if (!m_databasePath.isEmpty())
    {
        ui->lineEdit_databasepath->setText(m_databasePath);
    }
}

void Tablegenerator::on_pushButton_generate_clicked()
{
    ClassScanner scanner;
    ClassParser parser;

    QList<ScannedClassFile> files =
        scanner.scanAndReadClassFiles(m_classPath);

    for (const ScannedClassFile& file : files)
    {
        QList<ParsedClass> classes =
            parser.parseCppClasses(file.content);

        for (const ParsedClass& cls : classes)
        {
            qDebug() << "class:" << cls.name;

            for (const ParsedAttribute& attribute : cls.attributes)
            {
                qDebug()
                << "type:" << attribute.type
                << "name:" << attribute.name
                << "relationship:" << attribute.isRelation;
            }
        }
    }

}

