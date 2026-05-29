#ifndef TABLEGENERATOR_H
#define TABLEGENERATOR_H

#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include "../client/ollamaclient.h"
#include "../database/databasemanager.h"

namespace Ui {
class Tablegenerator;
}

class Tablegenerator : public QWidget
{
    Q_OBJECT

public:
    explicit Tablegenerator(DatabaseManager *databaseManager,
                            QWidget *parent = nullptr);
    ~Tablegenerator();

    void setSelectedModel(const QString& model);

private slots:
    void on_pushButton_addclasses_clicked();
    void on_pushButton_generate_clicked();

    void on_pushButton_execute_clicked();

private:
    Ui::Tablegenerator *ui;
    OllamaClient *m_ollamaClient = nullptr;
    DatabaseManager *m_dataBaseManager = nullptr;
    //QString m_databasePath;
    QString m_classPath;
    QString m_selectedModel;
    QString m_prompt;
};


#endif

