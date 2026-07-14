#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include <QSettings>
#include <QFileDialog>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <memory>

#include "client/ollamaenvironment.h"
#include "client/ollamaclient.h"
#include "database/databasemanager.h"
#include "utils/codegenerationprofile.h"
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
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint WRITE setOllamaEndpoint NOTIFY ollamaEndpointChanged)
    Q_PROPERTY(QString aiConnectionStatus READ aiConnectionStatus NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(QString aiSetupInstructions READ aiSetupInstructions NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(QStringList availableModels READ availableModels NOTIFY availableModelsChanged)
    Q_PROPERTY(bool ollamaInstalled READ ollamaInstalled NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(bool ollamaRunning READ ollamaRunning NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(bool aiEnvironmentReady READ aiEnvironmentReady NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(QString normalizationOutput READ normalizationOutput NOTIFY normalizationChanged)
    Q_PROPERTY(QString normalizationStatus READ normalizationStatus NOTIFY normalizationChanged)
    Q_PROPERTY(QString selectedNormalizationForm READ selectedNormalizationForm NOTIFY normalizationChanged)
    Q_PROPERTY(QString appliedNormalizationForm READ appliedNormalizationForm NOTIFY normalizationChanged)
    Q_PROPERTY(bool normalizationReady READ normalizationReady NOTIFY normalizationChanged)
    Q_PROPERTY(bool canResetNormalization READ canResetNormalization NOTIFY normalizationChanged)
    Q_PROPERTY(bool canAdvanceNormalization READ canAdvanceNormalization NOTIFY normalizationChanged)
    Q_PROPERTY(QVariantList normalizationBeforeSchema READ normalizationBeforeSchema NOTIFY normalizationChanged)
    Q_PROPERTY(QVariantList normalizationAfterSchema READ normalizationAfterSchema NOTIFY normalizationChanged)
    Q_PROPERTY(QVariantMap codeGenerationOptions READ codeGenerationOptions WRITE setCodeGenerationOptions NOTIFY codeGenerationSettingsChanged)
    Q_PROPERTY(bool generatedCodeValid READ generatedCodeValid NOTIFY generatedCodeValidationChanged)
    Q_PROPERTY(QString codeGenerationValidationSummary READ codeGenerationValidationSummary NOTIFY generatedCodeValidationChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    QString selectedModel() const;
    QString selectedLanguageName() const;
    QString dalOutputPath() const;
    QString classesFolderPath() const;
    QString lastSqlOutputPath() const;
    QString ollamaEndpoint() const;
    QString aiConnectionStatus() const;
    QString aiSetupInstructions() const;
    QStringList availableModels() const;
    bool databaseConnected() const;
    bool isLocalDatabase() const;
    bool executable() const;
    bool loading() const;
    bool ollamaInstalled() const;
    bool ollamaRunning() const;
    bool aiEnvironmentReady() const;
    QString normalizationOutput() const;
    QString normalizationStatus() const;
    QString selectedNormalizationForm() const;
    QString appliedNormalizationForm() const;
    bool normalizationReady() const;
    bool canResetNormalization() const;
    bool canAdvanceNormalization() const;
    QVariantList normalizationBeforeSchema() const;
    QVariantList normalizationAfterSchema() const;
    QVariantMap codeGenerationOptions() const;
    bool generatedCodeValid() const;
    QString codeGenerationValidationSummary() const;
    void setCodeGenerationOptions(const QVariantMap& options);

    Q_INVOKABLE QStringList codeLanguages() const;
    Q_INVOKABLE QStringList databaseDriverNames() const;
    Q_INVOKABLE QStringList normalizationForms() const;
    Q_INVOKABLE void setSelectedLanguage(int index);
    Q_INVOKABLE void setSelectedModel(const QString& model);
    Q_INVOKABLE void setOllamaEndpoint(const QString& endpoint);
    Q_INVOKABLE void setDalOutputPath(const QString& path);
    Q_INVOKABLE void setClassesFolderPath(const QString& path);
    Q_INVOKABLE void setIsLocalDatabase(bool isLocal);
    Q_INVOKABLE void onGenerateSqlCode();
    Q_INVOKABLE void onGenerateDalCode(bool secureAccess);
    Q_INVOKABLE void onGenerateApplicationCode(const QVariantMap& options);
    Q_INVOKABLE bool validateGeneratedCode(const QString& response);
    Q_INVOKABLE void onExportDalCode(const QString& response, const QString& outputPath);
    Q_INVOKABLE QString onExecuteSqlCode(const QString& response);
    Q_INVOKABLE void setAddAuditFields(bool enabled);
    Q_INVOKABLE void fetchModels();
    Q_INVOKABLE void refreshAiEnvironment();
    Q_INVOKABLE void connectDatabase(const QString& databasePath);
    Q_INVOKABLE void connectOnlineDatabase(
        const QString& driverName,
        const QString& databaseName,
        const QString& hostName,
        const QString& port,
        const QString& userName,
        const QString& password);
    Q_INVOKABLE void onGenerateNormalization(const QString& form);
    Q_INVOKABLE QString onApplyNormalization();
    Q_INVOKABLE QString onResetNormalization();
    Q_INVOKABLE QString onAdvanceNormalization();

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
    void ollamaEndpointChanged();
    void aiEnvironmentChanged();
    void availableModelsChanged();
    void dalStatusChanged(const QString& status);
    void dalExportFinished(const QString& message);
    void normalizationChanged();
    void codeGenerationSettingsChanged();
    void generatedCodeValidationChanged();

private:
    struct NormalizationVersion
    {
        QString form;
        QString migrationSql;
        QStringList tableNames;
    };

    void updateAiEnvironmentStatus();
    void loadNormalizationState(const QString& databaseIdentity);
    void saveNormalizationState();
    void handleNormalizationResponse(const QString& sql);
    int normalizationVersionIndex(const QString& form) const;
    QStringList activeNormalizationTables() const;
    QStringList sourceNormalizationTables() const;
    void prepareNormalizationVersion(
        int versionIndex,
        const QString& actionName);
    void clearNormalizationPreview(const QString& status);

    QString m_classFolderPath;
    QString m_selectedModel;
    QStringList m_availableModels;
    QString m_prompt;
    QString m_dalOutputPath;
    QString m_lastSqlOutputPath;
    QString m_ollamaEndpoint;
    QString m_aiConnectionStatus;
    QString m_aiSetupInstructions;
    QString m_normalizationOutput;
    QString m_normalizationStatus;
    QString m_normalizationPrompt;
    QString m_selectedNormalizationForm;
    QString m_appliedNormalizationForm;
    QString m_lastAppliedNormalizationSql;
    QString m_databaseIdentity;
    QString m_lastCodeGenerationPrompt;
    QString m_codeGenerationValidationSummary;
    QVariantList m_normalizationBeforeSchema;
    QVariantList m_normalizationAfterSchema;
    QVariantMap m_codeGenerationOptions;
    QStringList m_lastCodeGenerationTables;
    QVector<NormalizationVersion> m_normalizationHistory;

    OllamaClient *m_ollamaClient;
    std::unique_ptr<DatabaseManager> m_dataBaseManager;
    Tablegenerator *m_tableGenerator = nullptr;

    bool m_databaseConnected = false;
    bool m_isLocalDatabase = true;
    bool m_isExecutable = false;
    bool m_loading = false;
    bool m_addAuditFields = false;
    bool m_ollamaInstalled = false;
    bool m_ollamaRunning = false;
    bool m_aiEnvironmentReady = false;
    bool m_normalizationReady = false;
    bool m_generatedCodeValid = false;
    int m_normalizationRepairAttempts = 0;
    int m_codeGenerationRepairAttempts = 0;
    int m_activeNormalizationVersion = 0;
    int m_pendingNormalizationVersion = -1;

    ProgrammingLanguage::ProgrammingLanguageType m_selectedLanguageType = ProgrammingLanguage::ProgrammingLanguageType::Cplusplus;
    ProgrammingLanguage::ProgrammingLanguageType m_generationLanguageType = ProgrammingLanguage::ProgrammingLanguageType::Cplusplus;
};

#endif
