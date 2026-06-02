#ifndef TABLEGENERATOR_H
#define TABLEGENERATOR_H

#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include "../client/ollamaclient.h"
#include "../database/databasemanager.h"
#include "../utils/programminglanguage.h"

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
    void setSelectedLanguage(ProgrammingLanguage::ProgrammingLanguageType language);

private slots:
    void on_pushButton_addclasses_clicked();
    void on_pushButton_generate_clicked();

    void on_pushButton_execute_clicked();

    void on_pushButton_back_clicked();

    void on_pushButton_normalize_clicked();

signals:
    void windowClosed();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::Tablegenerator *ui;
    OllamaClient *m_ollamaClient = nullptr;
    DatabaseManager *m_dataBaseManager = nullptr;
    //QString m_databasePath;
    QString m_classPath;
    QString m_selectedModel;
    QString m_prompt;
    ProgrammingLanguage::ProgrammingLanguageType m_selectedLanguage =
        ProgrammingLanguage::ProgrammingLanguageType::Cplusplus;
};


#endif

