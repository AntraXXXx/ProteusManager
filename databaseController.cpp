#include "databaseController.h"
#include "ui_databaseController.h"

Databasecontroller::Databasecontroller(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Databasecontrollerform)
{
    ui->setupUi(this);
}

Databasecontroller::~Databasecontroller()
{
    delete ui;
}