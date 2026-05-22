#ifndef TABLEGENERATOR_H
#define TABLEGENERATOR_H

#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>
#include "../client/ollamaclient.h"

namespace Ui {
class Tablegenerator;
}

class Tablegenerator : public QWidget
{
    Q_OBJECT

public:
    explicit Tablegenerator(QWidget *parent = nullptr);
      ~Tablegenerator();

private slots:

    void on_pushButton_adddatabasedir_clicked();

    void on_pushButton_addclasses_clicked();

    void on_pushButton_generate_clicked();

private:
    Ui::Tablegenerator *ui;
    QString m_databasePath;
    QString m_classPath;
    OllamaClient *m_ollamaClient;
};


#endif

