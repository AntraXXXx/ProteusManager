#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include <QSettings>
#include <QFileDialog>

#include "client/ollamaclient.h"
#include "database/databasemanager.h"
#include "utils/programminglanguage.h"
#include "windows/tablegenerator.h"

class AppController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool databaseConnected READ databaseConnected NOTIFY databaseConnectedChanged)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(bool isLocalDatabase READ isLocalDatabase WRITE setIsLocalDatabase NOTIFY isLocalDatabaseChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    QString selectedModel() const;
    bool databaseConnected() const;
    bool isLocalDatabase() const;

    Q_INVOKABLE QStringList codeLanguages() const;
    Q_INVOKABLE void setSelectedLanguage(int index);
    Q_INVOKABLE void setSelectedModel(const QString& model);
    Q_INVOKABLE void setIsLocalDatabase(bool isLocal);
    Q_INVOKABLE void onGenerateSqlCode();
    Q_INVOKABLE void onGenerateDalCode(bool secureAccess);
    Q_INVOKABLE void onExportDalCode(const QString& response, const QString& outputPath);
    Q_INVOKABLE void setClassesFolderPath(const QString& path);
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
    void sqlOutputChanged(const QString& text);
    void sqlGenerationLoadingChanged(bool loading);
    void sqlGenerateEnabledChanged(bool enabled);
    void warningOccurred(const QString& title, const QString& message);
    void classesFolderPathChanged(const QString& path);
    void dalOutputChanged(const QString& code);
    void dalStatusChanged(const QString& status);
    void dalExportFinished(const QString& message);

private:
    QString m_classPath;
    QString m_selectedModel;
    QString m_prompt;

    OllamaClient *m_ollamaClient;
    DatabaseManager *m_dataBaseManager;
    Tablegenerator *m_tableGenerator;

    bool m_databaseConnected = false;
    bool m_isLocalDatabase = true;

    bool m_addAuditFields = false;

    ProgrammingLanguage::ProgrammingLanguageType m_selectedLanguageType = ProgrammingLanguage::ProgrammingLanguageType::Cplusplus;
};

#endif