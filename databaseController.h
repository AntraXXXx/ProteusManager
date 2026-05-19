#ifndef Databasecontroller_H
#define Databasecontroller_H

#include <QWidget>

namespace Ui {
class Databasecontrollerform;
}

class Databasecontroller : public QWidget
{
    Q_OBJECT

public:
    explicit Databasecontroller(QWidget *parent = nullptr);
    ~Databasecontroller();

private:
    Ui::Databasecontrollerform *ui;
};

#endif
