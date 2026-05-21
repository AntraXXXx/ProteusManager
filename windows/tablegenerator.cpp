#include "tablegenerator.h"
#include "ui_tablegenerator.h"
#include "../utils/textfilemanager.h"
#include "../utils/classscanner.h"

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
    TextFileManager fileManager;

 //   QString dataBaseContent =
      //  fileManager.readFile(m_databasePath);

   // QString classContent =
      //  fileManager.readFile(m_classPath);

    ClassScanner scanner;

    QStringList files =
        scanner.scanClassFiles(m_classPath);

}

