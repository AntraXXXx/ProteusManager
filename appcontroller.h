#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include <QSettings>
#include <QFileDialog>

#include <memory>

#include "client/ollamaclient.h"
#include "database/databasemanager.h"
#include "utils/dalfileexporter.h"
#include "utils/programminglanguage.h"
#include "utils/sqloutputrecorder.h"
#include "windows/tablegenerator.h"

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString selectedLanguageName READ selectedLanguageName NOTIFY languageChanged)
    Q_PROPERTY(bool executable READ executable NOTIFY executableChanged)
    Q_PROPERTY(bool databaseConnected READ databaseConnected NOTIFY databaseConnectedChanged)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(bool isLocalDatabase READ isLocalDatabase WRITE setIsLocalDatabase NOTIFY isLocalDatabaseChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString dalOutputPath READ dalOutputPath NOTIFY dalOutputPathChanged)
    Q_PROPERTY(QString classesFolderPath READ classesFolderPath NOTIFY classesFolderPathChanged)
    Q_PROPERTY(QString lastSqlOutputPath READ lastSqlOutputPath NOTIFY lastSqlOutputPathChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    QString selectedModel() const;
    QString selectedLanguageName() const;
    QString dalOutputPath() const;
    QString classesFolderPath() const;
    QString lastSqlOutputPath() const;
    bool databaseConnected() const;
    bool isLocalDatabase() const;
    bool executable() const;
    bool loading() const;

    Q_INVOKABLE QStringList codeLanguages() const;
    Q_INVOKABLE void setSelectedLanguage(int index);
    Q_INVOKABLE void setSelectedModel(const QString& model);
    Q_INVOKABLE void setDalOutputPath(const QString& path);
    Q_INVOKABLE void setClassesFolderPath(const QString& path);
    Q_INVOKABLE void setIsLocalDatabase(bool isLocal);
    Q_INVOKABLE void onGenerateSqlCode();
    Q_INVOKABLE void onGenerateDalCode(bool secureAccess);
    Q_INVOKABLE void onExportDalCode(const QString& response, const QString& outputPath);
    Q_INVOKABLE QString onExecuteSqlCode(const QString& response);
    Q_INVOKABLE void setAddAuditFields(bool enabled);
    Q_INVOKABLE void fetchModels();
    Q_INVOKABLE void connectDatabase(const QString& databasePath);

signals:
    void modelsFetched(const QStringList& models);
    void databaseConnectedChanged(bool connected);
    void databaseStatusChanged(const QString& status);
    void databasePathChanged(const QString& path);
    void selectedModelChanged();
    void isLocalDatabaseChanged();
    void languageChanged();
    void executableChanged();
    void loadingChanged();
    void sqlOutputChanged(const QString& text);
    void warningOccurred(const QString& title, const QString& message);
    void dalOutputChanged(const QString& path);
    void dalOutputPathChanged();
    void classesFolderPathChanged();
    void lastSqlOutputPathChanged();
    void sqlOutputSaved(const QString& path);
    void dalStatusChanged(const QString& status);
    void dalExportFinished(const QString& message);

private:
    QString m_classFolderPath;
    QString m_selectedModel;
    QString m_prompt;
    QString m_dalOutputPath;
    QString m_lastSqlOutputPath;

    OllamaClient *m_ollamaClient;
    std::unique_ptr<DatabaseManager> m_dataBaseManager;
    Tablegenerator *m_tableGenerator = nullptr;

    bool m_databaseConnected = false;
    bool m_isLocalDatabase = true;
    bool m_isExecutable = false;
    bool m_loading = false;
    bool m_addAuditFields = false;

    ProgrammingLanguage::ProgrammingLanguageType m_selectedLanguageType = ProgrammingLanguage::ProgrammingLanguageType::Cplusplus;
};

#endif
