#include "tablegenerator.h"
#include "ui_tablegenerator.h"

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
    QString path =
        QFileDialog::getOpenFileName(
            this,
            "Datenbank auswählen",
            "",
            "SQLite Database (*.db *.sqlite)"
            );

    if (!path.isEmpty())
    {
        ui->lineEdit_databasepath->setText(path);
    }
}

void Tablegenerator::on_pushButton_adddatabasedir_clicked()
{
    QString path =
        QFileDialog::getOpenFileName(
            this,
            "Skript auswählen",
            "",
            "Skript (*.h *.cs)" // c++ and c#?
            );

    if (!path.isEmpty())
    {
        ui->lineEdit_databasepath->setText(path);
    }
}