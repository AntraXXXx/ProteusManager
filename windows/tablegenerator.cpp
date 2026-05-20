#include "tablegenerator.h"
#include "ui_tablegenerator.h"
#include "../utils/textfilemanager.h"

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
    m_databasePath =
        QFileDialog::getOpenFileName(
            this,
            "Datenbank auswählen",
            "",
            "SQLite Database (*.db *.sqlite)"
            );

    if (!m_databasePath.isEmpty())
    {
        ui->lineEdit_databasepath->setText(m_databasePath);
    }
}

void Tablegenerator::on_pushButton_adddatabasedir_clicked()
{
    m_classPath =
        QFileDialog::getOpenFileName(
            this,
            "Skript auswählen",
            "",
            "Skript (*.h *.cs)" // c++ and c#?
            );

    if (!m_classPath.isEmpty())
    {
        ui->lineEdit_databasepath->setText(m_classPath);
    }
}
void Tablegenerator::on_pushButton_generate_clicked()
{
    TextFileManager fileManager;

    QString dataBaseContent =
        fileManager.readFile(m_databasePath);

    QString classContent =
        fileManager.readFile(m_classPath);
}

