#ifndef DATABASECONTROLLER_H
#define DATABASECONTROLLER_H

#include <QWidget>

namespace Ui {
class DatabaseController;
}

class DatabaseController : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseController(QWidget *parent = nullptr);
    ~DatabaseController();

private slots:
    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::DatabaseController *ui;
};

#endif
