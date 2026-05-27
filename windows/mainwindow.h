#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//#include "databasecontroller.h"
#include "../windows/tablegenerator.h"
#include "../client/ollamaclient.h"
#include "../database/databasemanager.h"
#include <QSettings>
#include <QGroupBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_pushButton_SqlGenerator_clicked();

    void on_comboBox_AiModell_currentIndexChanged(int index);

    void on_checkBox_isLocalDatabase_checkStateChanged(const Qt::CheckState &arg1);

    void on_pushButton_adddatabasedir_clicked();

    void on_pushButton_ConnectDB_clicked();

private:
    Ui::MainWindow *ui;
    OllamaClient *m_ollamaClient;
    DatabaseManager *m_dataBaseManager = nullptr;
   // DatabaseController *m_databaseController = nullptr;
    Tablegenerator *m_tableGenerator = nullptr;
    QString m_selectedModel;
};
#endif
