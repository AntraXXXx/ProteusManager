#include "databasecontroller.h"
#include "ui_databasecontroller.h"

DatabaseController::DatabaseController(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DatabaseController)
{
    ui->setupUi(this);
}

DatabaseController::~DatabaseController()
{
    delete ui;
}

void DatabaseController::on_pushButton_3_clicked()
{

}


void DatabaseController::on_pushButton_2_clicked()
{

}

