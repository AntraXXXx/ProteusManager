#include "appcontroller.h"

#include <algorithm>

namespace
{
QString driverKeyFromDisplayName(const QString& driverName)
{
    if (driverName.contains("PostgreSQL", Qt::CaseInsensitive))
        return "QPSQL";

    if (driverName.contains("SQL Server", Qt::CaseInsensitive)
        || driverName.contains("ODBC", Qt::CaseInsensitive))
    {
        return "QODBC";
    }

    return "QMYSQL";
}

QString databaseDialectName(const QString& driverName)
{
    if (driverName == "QSQLITE")
        return "SQLite 3";

    if (driverName == "QMYSQL")
        return "MySQL or MariaDB";

    if (driverName == "QPSQL")
        return "PostgreSQL";

    if (driverName == "QODBC")
        return "Microsoft SQL Server or ODBC";

    return "the connected database";
}

QString normalizationDialectRules(const QString& driverName)
{
    if (driverName == "QSQLITE")
    {
        return
            "SQLite rules: use instr(value, delimiter), substr(), || for string concatenation, "
            "and WITH RECURSIVE when rows must be split. Never use CHARINDEX, STRING_SPLIT, "
            "SPLIT_PART, TOP, AUTO_INCREMENT or IDENTITY. ";
    }

    if (driverName.startsWith("QMYSQL"))
    {
        return
            "MySQL/MariaDB rules: use INSTR(), SUBSTRING(), CONCAT() and supported recursive CTE syntax. "
            "Never use CHARINDEX, STRING_SPLIT, SPLIT_PART, TOP or IDENTITY. ";
    }

    if (driverName.startsWith("QPSQL"))
    {
        return
            "PostgreSQL rules: use POSITION(delimiter IN value) or STRPOS(), SUBSTRING(), ||, "
            "and PostgreSQL recursive CTE syntax. Never use INSTR, IFNULL, CHARINDEX, "
            "STRING_SPLIT, backtick identifiers, AUTO_INCREMENT or IDENTITY. ";
    }

    if (driverName.startsWith("QODBC"))
    {
        return
            "Microsoft SQL Server rules: use CHARINDEX(), SUBSTRING(), + for string concatenation, "
            "and SQL Server CTE syntax. Never use INSTR, SUBSTR, STRPOS, SPLIT_PART, LIMIT, "
            "backtick identifiers, || or AUTO_INCREMENT. ";
    }

    return {};
}

QString normalizationGoal(const QString& form)
{
    if (form == "1NF")
        return "Make every value atomic and remove repeating groups.";

    if (form == "2NF")
        return "Satisfy 1NF and remove partial dependencies on composite keys.";

    if (form == "3NF")
        return "Satisfy 2NF and remove transitive dependencies.";

    if (form == "BCNF")
        return "Ensure every determinant is a candidate key.";

    if (form == "4NF")
        return "Satisfy BCNF and remove independent multivalued dependencies.";

    if (form == "5NF")
        return "Satisfy 4NF and remove non-key join dependencies while preserving lossless joins.";

    return {};
}

QString codeGenerationSettingsKey(
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    return QString("codeGeneration/options/%1")
        .arg(static_cast<int>(language));
}
}

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_ollamaClient(new OllamaClient(this))
    , m_dataBaseManager(std::make_unique<DatabaseManager>())
{
    QSettings settings("DataBaseSettings", "Proteus");

    m_isLocalDatabase = settings.value("database/isLocalConnection", true).toBool();
    m_dalOutputPath = settings.value("dal/outputPath","").toString();
    m_classFolderPath = settings.value("classes/scripts", "").toString();
    QVariantMap storedCodeGenerationOptions =
        settings.value(
            codeGenerationSettingsKey(m_selectedLanguageType))
            .toMap();
    if (storedCodeGenerationOptions.isEmpty())
    {
        storedCodeGenerationOptions =
            settings.value("codeGeneration/options").toMap();
    }
    m_codeGenerationOptions =
        CodeGenerationProfile::optionsFor(
            m_selectedLanguageType,
            storedCodeGenerationOptions)
            .toVariantMap();
    m_codeGenerationValidationSummary =
        "Generate code to run structural and security validation.";
    m_ollamaEndpoint =
        settings.value(
            "ai/ollamaEndpoint",
            OllamaEnvironment::defaultEndpoint()
            ).toString();
    m_normalizationStatus =
        "No normalization has been applied.";

    m_ollamaClient->setBaseUrl(QUrl(m_ollamaEndpoint));
   // emit dalOutputPathChanged(m_classPath);
    emit dalOutputPathChanged();
    emit classesFolderPathChanged();
    emit ollamaEndpointChanged();

    connect(
        m_ollamaClient,
        &OllamaClient::connectionChecked,
        this,
        [this](bool isRunning)
        {
            m_ollamaRunning = isRunning;

            if (isRunning)
            {
                m_aiConnectionStatus =
                    "Connected to Ollama API.";
                updateAiEnvironmentStatus();
                m_ollamaClient->fetchModels();
                return;
            }

            m_availableModels.clear();
            m_selectedModel.clear();
            m_aiConnectionStatus =
                "Ollama API is not reachable.";
            updateAiEnvironmentStatus();
            emit selectedModelChanged();
            emit modelsFetched(m_availableModels);
            emit availableModelsChanged();
        });

    connect(
        m_ollamaClient,
        &OllamaClient::errorOccurred,
        this,
        [this](const QString& errorMessage)
        {
            m_ollamaRunning = false;
            m_availableModels.clear();
            m_selectedModel.clear();
            m_aiConnectionStatus =
                "Ollama connection failed: " + errorMessage;
            updateAiEnvironmentStatus();
            emit selectedModelChanged();
            emit modelsFetched(m_availableModels);
            emit availableModelsChanged();

            if (m_loading)
            {
                m_loading = false;
                emit loadingChanged();
            }

            if (m_codeAssistantBusy)
            {
                m_codeAssistantBusy = false;
                m_codeAssistantMessages.append(QVariantMap{
                    {"role", "assistant"},
                    {"text", "Assistant request failed: " + errorMessage}
                });
                emit codeAssistantChanged();
            }

            if (!m_selectedNormalizationForm.isEmpty())
            {
                m_normalizationReady = false;
                m_normalizationStatus =
                    "Normalization request failed: "
                    + errorMessage;
                emit normalizationChanged();
            }
        });

    // SqlReceived
    connect(
        m_ollamaClient,
        &OllamaClient::sqlReceived,
        this,
        [this](const QString& sql)
        {
            emit sqlOutputChanged(sql);

            const SqlOutputRecord record =
                SqlOutputRecorder::recordGeneratedSql(
                    m_selectedModel,
                    sql);

            if (record.saved)
            {
                m_lastSqlOutputPath = record.filePath;
                emit lastSqlOutputPathChanged();
                emit sqlOutputSaved(record.filePath);
            }

            QSettings settings("DataBaseSettings", "Proteus");
            m_classFolderPath = settings.value("classes/scripts").toString();
            emit classesFolderPathChanged();

            bool valid =
                m_dataBaseManager->isValidSql(sql);

            if (m_isExecutable != valid)
            {
                m_isExecutable = valid;
                emit executableChanged();
            }

            m_loading = false;
            emit loadingChanged();
        });

    connect(
        m_ollamaClient,
        &OllamaClient::modelsFetched,
        this,
        [this](const QStringList& models)
        {
            m_availableModels = models;

            if (!models.isEmpty())
            {
                if (m_selectedModel.isEmpty()
                    || !models.contains(m_selectedModel))
                {
                    m_selectedModel = models.first();
                    emit selectedModelChanged();
                }

                m_aiConnectionStatus =
                    QString("AI ready. Models installed: %1")
                        .arg(models.size());
            }
            else
            {
                m_selectedModel.clear();
                emit selectedModelChanged();
                m_aiConnectionStatus =
                    "Ollama is running, but no model is installed.";
            }

            updateAiEnvironmentStatus();
            emit modelsFetched(m_availableModels);
            emit availableModelsChanged();
        });

    connect(
        m_ollamaClient,
        &OllamaClient::dalReceived,
        this,
        [this](const QString& code)
        {
            const QString normalizedCode =
                DalFileExporter::applySqlNamingConvention(
                    code,
                    m_lastCodeGenerationTables);

            const CodeGenerationOptions options =
                CodeGenerationProfile::optionsFor(
                    m_generationLanguageType,
                    m_codeGenerationOptions);
            const QStringList validationErrors =
                CodeGenerationProfile::validateResponse(
                    normalizedCode,
                    m_generationLanguageType,
                    options,
                    m_lastCodeGenerationTables);

            if (!validationErrors.isEmpty())
            {
                m_generatedCodeValid = false;
                m_codeGenerationValidationSummary =
                    "Generated code failed validation:\n- "
                    + validationErrors.join("\n- ");
                emit generatedCodeValidationChanged();

                if (m_codeGenerationRepairAttempts == 0
                    && m_aiEnvironmentReady)
                {
                    ++m_codeGenerationRepairAttempts;
                    emit dalStatusChanged(
                        "Generated code failed validation. Asking the AI to repair the complete file set...");

                    const QString repairPrompt =
                        m_lastCodeGenerationPrompt
                        + "\n\nThe previous response failed validation for these reasons:\n- "
                        + validationErrors.join("\n- ")
                        + "\nRegenerate the complete file set. Fix every validation error and return all files again.";

                    m_ollamaClient->generate(
                        m_selectedModel,
                        repairPrompt,
                        OllamaClient::GenerateType::Dal);
                    return;
                }

                emit dalOutputChanged(normalizedCode);
                emit dalStatusChanged(
                    "Generated code was rejected. Review the validation details before trying again.");
                m_loading = false;
                emit loadingChanged();
                return;
            }

            m_generatedCodeValid = true;
            m_codeGenerationValidationSummary =
                "Generated code passed file, layer and parameter-binding validation.";
            emit generatedCodeValidationChanged();

            emit dalOutputChanged(normalizedCode);
            QSettings settings("DataBaseSettings", "Proteus");
            m_dalOutputPath = settings.value("dal/outputPath").toString();
            emit dalOutputPathChanged();
            m_isExecutable = true;
            emit executableChanged();
            m_loading = false;
            emit loadingChanged();
            emit dalStatusChanged(
                "Code generation completed and validation passed.");
        });

    connect(
        m_ollamaClient,
        &OllamaClient::normalizationReceived,
        this,
        [this](const QString& sql)
        {
            handleNormalizationResponse(sql);
        });

    connect(
        m_ollamaClient,
        &OllamaClient::assistantReceived,
        this,
        [this](const QString& response)
        {
            m_codeAssistantMessages.append(QVariantMap{
                {"role", "assistant"},
                {"text", response.isEmpty()
                             ? "The assistant returned an empty response."
                             : response}
            });
            m_codeAssistantBusy = false;
            emit codeAssistantChanged();
        });

    refreshAiEnvironment();
    m_isExecutable = true;
    emit executableChanged();
    m_loading = false;
    emit loadingChanged();
}

bool AppController::databaseConnected() const
{
    return m_databaseConnected;
}

bool AppController::loading() const
{
    return m_loading;
}

QString AppController::selectedModel() const
{
    return m_selectedModel;
}

QString AppController::dalOutputPath() const
{
    return m_dalOutputPath;
}

QString AppController::classesFolderPath() const
{
    return m_classFolderPath;
}

QString AppController::lastSqlOutputPath() const
{
    return m_lastSqlOutputPath;
}

QString AppController::ollamaEndpoint() const
{
    return m_ollamaEndpoint;
}

QString AppController::aiConnectionStatus() const
{
    return m_aiConnectionStatus;
}

QString AppController::aiSetupInstructions() const
{
    return m_aiSetupInstructions;
}

QStringList AppController::availableModels() const
{
    return m_availableModels;
}

bool AppController::isLocalDatabase() const
{
    return m_isLocalDatabase;
}

void AppController::fetchModels()
{
    refreshAiEnvironment();
}

void AppController::refreshAiEnvironment()
{
    const OllamaInstallationStatus installation =
        OllamaEnvironment::detectInstallation();

    m_ollamaInstalled = installation.installed;
    m_ollamaRunning = false;
    m_aiEnvironmentReady = false;
    m_availableModels.clear();

    if (!OllamaEnvironment::isEndpointValid(m_ollamaEndpoint))
    {
        m_aiConnectionStatus =
            "Invalid Ollama endpoint.";
        updateAiEnvironmentStatus();
        emit modelsFetched(m_availableModels);
        emit availableModelsChanged();
        return;
    }

    m_aiConnectionStatus =
        "Checking Ollama environment...";
    updateAiEnvironmentStatus();

    m_ollamaClient->setBaseUrl(QUrl(m_ollamaEndpoint));
    m_ollamaClient->checkConnection();
}

void AppController::updateAiEnvironmentStatus()
{
    m_aiEnvironmentReady =
        m_ollamaRunning
        && !m_availableModels.isEmpty();

    m_aiSetupInstructions =
        OllamaEnvironment::setupInstructions(
            m_ollamaInstalled,
            m_ollamaRunning,
            m_availableModels);

    emit aiEnvironmentChanged();
}

void AppController::handleNormalizationResponse(
    const QString& sql)
{
    m_normalizationOutput = sql.trimmed();
    const bool noChangesRequired =
        m_normalizationOutput.compare(
            "NO_CHANGES_REQUIRED",
            Qt::CaseInsensitive) == 0;
    m_normalizationReady =
        !noChangesRequired
        && m_dataBaseManager->validateMigrationPreview(
            m_normalizationOutput);

    if (noChangesRequired)
    {
        if (m_dataBaseManager->hasNormalizationEvidence(
                m_normalizationInputTables.isEmpty()
                    ? sourceNormalizationTables()
                    : m_normalizationInputTables))
        {
            if (m_normalizationRepairAttempts == 0
                && m_aiEnvironmentReady)
            {
                ++m_normalizationRepairAttempts;
                m_normalizationStatus =
                    "The AI returned NO_CHANGES_REQUIRED, but the schema still contains normalization evidence. Asking the AI to generate a real migration...";
                emit normalizationChanged();

                const QString repairPrompt =
                    m_normalizationPrompt
                    + "\n\nYour previous answer was NO_CHANGES_REQUIRED, but the local analyzer found normalization evidence. "
                      "This includes denormalized table names, numbered repeating column groups such as produkt_1_name/produkt_2_name, or aligned list values. "
                      "Generate a real, lossless migration for "
                    + m_selectedNormalizationForm
                    + ". Do not return NO_CHANGES_REQUIRED unless no repeating groups, no transitive dependencies and no denormalized naming evidence remain. "
                      "Preserve all existing data and keep the SQL dialect rules from the original request.";

                m_ollamaClient->generate(
                    m_selectedModel,
                    repairPrompt,
                    OllamaClient::GenerateType::Normalization);
                return;
            }

            m_normalizationReady = false;
            m_normalizationAfterSchema =
                m_appliedNormalizationForm.isEmpty()
                    ? QVariantList{}
                    : m_dataBaseManager->buildSchemaDiagramForTables(
                          activeNormalizationTables());
            m_normalizationStatus =
                "NO_CHANGES_REQUIRED was rejected because the schema still contains normalization evidence.";
            m_loading = false;
            emit loadingChanged();
            emit normalizationChanged();
            return;
        }

        m_normalizationReady = true;
        m_normalizationAfterSchema =
            m_normalizationBeforeSchema;
        m_normalizationStatus =
            "The active schema already satisfies "
            + m_selectedNormalizationForm
            + ". Apply Normalization to register this version without executing SQL.";
    }
    else if (m_normalizationReady)
    {
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramWithMigration(
                m_normalizationOutput);
        m_normalizationStatus =
            m_selectedNormalizationForm
            + " preview is ready. The database remains unchanged until Apply Normalization.";
    }
    else
    {
        const QString validationError =
            m_dataBaseManager->lastError();

        if (m_normalizationRepairAttempts == 0
            && m_aiEnvironmentReady)
        {
            ++m_normalizationRepairAttempts;
            m_normalizationStatus =
                "The first migration did not match the selected SQL dialect. Asking the AI to correct it...";
            emit normalizationChanged();

            const QString repairPrompt =
                m_normalizationPrompt
                + "\n\nThe previous migration failed validation for the connected database. "
                  "Correct the SQL without changing the requested normal form or losing data. "
                  "Validation error: "
                + validationError
                + "\nPrevious migration:\n"
                + m_normalizationOutput;
            m_ollamaClient->generate(
                m_selectedModel,
                repairPrompt,
                OllamaClient::GenerateType::Normalization);
            return;
        }

        m_normalizationAfterSchema =
            m_appliedNormalizationForm.isEmpty()
                ? QVariantList{}
                : m_dataBaseManager->buildSchemaDiagramForTables(
                      activeNormalizationTables());
        m_normalizationStatus =
            "The generated migration was rejected: "
            + validationError;
    }

    m_loading = false;
    emit loadingChanged();
    emit normalizationChanged();
}

int AppController::normalizationVersionIndex(
    const QString& form) const
{
    for (int i = 0; i < m_normalizationHistory.size(); ++i)
    {
        if (m_normalizationHistory.at(i).form.compare(
                form,
                Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }

    return -1;
}

QStringList AppController::activeNormalizationTables() const
{
    if (m_activeNormalizationVersion < 0
        || m_activeNormalizationVersion >= m_normalizationHistory.size())
    {
        return {};
    }

    return m_normalizationHistory.at(
        m_activeNormalizationVersion).tableNames;
}

QStringList AppController::sourceNormalizationTables() const
{
    if (!m_normalizationHistory.isEmpty())
        return m_normalizationHistory.first().tableNames;

    return m_dataBaseManager->getTableNames();
}

int AppController::storeNormalizationVersion(
    const NormalizationVersion& version)
{
    QStringList existingForms;
    for (const NormalizationVersion& existingVersion :
         m_normalizationHistory)
    {
        existingForms.append(existingVersion.form);
    }

    const int insertionIndex =
        NormalizationPlanner::insertionIndex(
            version.form,
            existingForms);
    m_normalizationHistory.insert(
        insertionIndex,
        version);
    return insertionIndex;
}

bool AppController::refreshNormalizationDiagrams()
{
    if (!m_dataBaseManager->isConnected())
    {
        m_normalizationStatus =
            "Connect a database before opening a schema diagram.";
        emit normalizationChanged();
        return false;
    }

    m_normalizationBeforeSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            sourceNormalizationTables());
    if (m_normalizationBeforeSchema.isEmpty())
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagram();

    if (m_pendingNormalizationVersion >= 0
        && m_pendingNormalizationVersion
               < m_normalizationHistory.size())
    {
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramForTables(
                m_normalizationHistory.at(
                    m_pendingNormalizationVersion).tableNames);
    }
    else if (m_normalizationReady
             && !m_normalizationOutput.isEmpty()
             && m_normalizationOutput.compare(
                    "NO_CHANGES_REQUIRED",
                    Qt::CaseInsensitive) != 0)
    {
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramWithMigration(
                m_normalizationOutput);
    }
    else if (!m_appliedNormalizationForm.isEmpty())
    {
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramForTables(
                activeNormalizationTables());
    }

    if (m_normalizationBeforeSchema.isEmpty())
    {
        m_normalizationStatus =
            "The connected database does not contain any tables. Create or connect a schema before normalization.";
        emit normalizationChanged();
        return false;
    }

    emit normalizationChanged();
    return true;
}

void AppController::saveNormalizationState()
{
    QVariantList storedVersions;
    for (const NormalizationVersion& version : m_normalizationHistory)
    {
        storedVersions.append(QVariantMap{
            {"form", version.form},
            {"migrationSql", version.migrationSql},
            {"tableNames", version.tableNames}
        });
    }

    QSettings settings("DataBaseSettings", "Proteus");
    settings.setValue("database/normalizationIdentity", m_databaseIdentity);
    settings.setValue("database/normalizationHistory", storedVersions);
    settings.setValue(
        "database/activeNormalizationVersion",
        m_activeNormalizationVersion);
    settings.setValue(
        "database/appliedNormalizationForm",
        m_appliedNormalizationForm);
    settings.setValue(
        "database/lastNormalizationMigration",
        m_lastAppliedNormalizationSql);
}

void AppController::prepareNormalizationVersion(
    int versionIndex,
    const QString& actionName)
{
    if (versionIndex < 0
        || versionIndex >= m_normalizationHistory.size())
    {
        m_normalizationStatus =
            "The requested normalization version is not available.";
        emit normalizationChanged();
        return;
    }

    if (m_dataBaseManager->getTableNames().isEmpty())
    {
        m_normalizationStatus =
            "The connected database does not contain any tables. Create or connect a schema before normalization.";
        m_normalizationBeforeSchema.clear();
        m_normalizationAfterSchema.clear();
        emit normalizationChanged();
        return;
    }

    const NormalizationVersion& version =
        m_normalizationHistory.at(versionIndex);
    m_pendingNormalizationVersion = versionIndex;
    m_selectedNormalizationForm = version.form;
    m_normalizationInputTables.clear();
    m_normalizationOutput =
        version.form.isEmpty()
            ? "Reactivate the original schema version. No SQL execution is required."
            : "Reactivate the existing " + version.form
                  + " schema version. No SQL execution is required.";
    m_normalizationBeforeSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            sourceNormalizationTables());
    if (m_normalizationBeforeSchema.isEmpty())
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagram();
    m_normalizationAfterSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            version.tableNames);
    m_normalizationReady = true;
    m_loading = false;
    m_normalizationStatus =
        actionName
        + " preview is ready. Apply Normalization activates this version without deleting data.";
    emit loadingChanged();
    emit normalizationChanged();
}

void AppController::clearNormalizationPreview(
    const QString& status)
{
    m_pendingNormalizationVersion = -1;
    m_selectedNormalizationForm.clear();
    m_normalizationInputTables.clear();
    m_normalizationOutput.clear();
    m_normalizationReady = false;
    m_loading = false;
    m_normalizationBeforeSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            sourceNormalizationTables());
    if (m_normalizationBeforeSchema.isEmpty())
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagram();

    if (m_appliedNormalizationForm.isEmpty())
    {
        m_normalizationAfterSchema.clear();
    }
    else
    {
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramForTables(
                activeNormalizationTables());
    }

    m_normalizationStatus = status;
    emit loadingChanged();
    emit normalizationChanged();
}

void AppController::loadNormalizationState(
    const QString& databaseIdentity)
{
    m_databaseIdentity = databaseIdentity;
    QSettings settings("DataBaseSettings", "Proteus");
    const QString savedIdentity =
        settings.value(
            "database/normalizationIdentity",
            "").toString();
    m_normalizationHistory.clear();

    if (savedIdentity == m_databaseIdentity)
    {
        const QVariantList storedVersions =
            settings.value(
                "database/normalizationHistory")
                .toList();
        for (const QVariant& storedValue : storedVersions)
        {
            const QVariantMap stored = storedValue.toMap();
            NormalizationVersion version;
            version.form = stored.value("form").toString();
            version.migrationSql =
                stored.value("migrationSql").toString();
            version.tableNames =
                stored.value("tableNames").toStringList();
            if (!version.tableNames.isEmpty())
                m_normalizationHistory.append(version);
        }

        m_activeNormalizationVersion =
            settings.value(
                "database/activeNormalizationVersion",
                0).toInt();

        if (m_normalizationHistory.isEmpty())
        {
            const QString legacyForm =
                settings.value(
                    "database/appliedNormalizationForm")
                    .toString();
            const QString legacyMigration =
                settings.value(
                    "database/lastNormalizationMigration")
                    .toString();

            if (!legacyForm.isEmpty()
                && !legacyMigration.isEmpty())
            {
                QStringList originalTables =
                    m_dataBaseManager->getTableNames();
                const QStringList targetTables =
                    m_dataBaseManager->migrationTargetTableNames(
                        legacyMigration);
                for (int i = originalTables.size() - 1; i >= 0; --i)
                {
                    if (targetTables.contains(
                            originalTables.at(i),
                            Qt::CaseInsensitive))
                    {
                        originalTables.removeAt(i);
                    }
                }

                m_normalizationHistory.append({
                    {}, {}, originalTables
                });
                m_normalizationHistory.append({
                    legacyForm,
                    legacyMigration,
                    targetTables
                });
                m_activeNormalizationVersion = 1;
            }
        }
    }

    if (m_normalizationHistory.isEmpty())
    {
        m_normalizationHistory.append({
            {}, {}, m_dataBaseManager->getTableNames()
        });
        m_activeNormalizationVersion = 0;
    }

    const int storedActiveIndex =
        qBound(
            0,
            m_activeNormalizationVersion,
            m_normalizationHistory.size() - 1);
    const QString storedActiveForm =
        m_normalizationHistory.at(storedActiveIndex).form;
    std::stable_sort(
        m_normalizationHistory.begin(),
        m_normalizationHistory.end(),
        [](const NormalizationVersion& left,
           const NormalizationVersion& right)
        {
            return NormalizationPlanner::formRank(left.form)
                   < NormalizationPlanner::formRank(right.form);
        });
    m_activeNormalizationVersion =
        qMax(0, normalizationVersionIndex(storedActiveForm));
    const NormalizationVersion& activeVersion =
        m_normalizationHistory.at(m_activeNormalizationVersion);
    m_appliedNormalizationForm = activeVersion.form;
    m_lastAppliedNormalizationSql = activeVersion.migrationSql;

    m_selectedNormalizationForm.clear();
    m_normalizationOutput.clear();
    m_normalizationReady = false;
    m_pendingNormalizationVersion = -1;
    m_normalizationInputTables.clear();
    m_normalizationBeforeSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            sourceNormalizationTables());
    if (m_normalizationBeforeSchema.isEmpty())
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagram();
    m_normalizationAfterSchema =
        m_appliedNormalizationForm.isEmpty()
            ? QVariantList{}
            : m_dataBaseManager->buildSchemaDiagramForTables(
                  activeVersion.tableNames);
    if (m_dataBaseManager->getTableNames().isEmpty())
    {
        m_normalizationStatus =
            "The connected database does not contain any tables. Create or connect a schema before normalization.";
    }
    else
    {
        m_normalizationStatus =
            m_appliedNormalizationForm.isEmpty()
                ? "No normalization has been applied."
                : "Applied normalization: "
                      + m_appliedNormalizationForm;
    }

    saveNormalizationState();
    emit normalizationChanged();
}

void AppController::connectDatabase(const QString& databasePath)
{
    QFileInfo fileInfo(databasePath);

    QString connectionName = fileInfo.fileName();
    QString filePath = databasePath;

    if (connectionName.isEmpty() || filePath.isEmpty())
    {
        m_databaseConnected = false;

        emit databaseStatusChanged("No database selected.");
        emit databaseConnectedChanged(false);

        return;
    }

    m_dataBaseManager->openDatabase(connectionName, filePath);

    m_databaseConnected =
        m_dataBaseManager->isConnected();

    if (m_databaseConnected)
    {
        loadNormalizationState(
            QFileInfo(filePath).absoluteFilePath());
        emit databaseStatusChanged("Connected");
    }
    else
    {
        emit databaseStatusChanged("Connection failed");
    }

    emit databaseConnectedChanged(m_databaseConnected);
}

void AppController::connectOnlineDatabase(
    const QString& driverName,
    const QString& databaseName,
    const QString& hostName,
    const QString& port,
    const QString& userName,
    const QString& password)
{
    const QString driver =
        driverKeyFromDisplayName(driverName);

    bool portIsNumber = false;
    const int portNumber =
        port.trimmed().toInt(&portIsNumber);

    if (!port.trimmed().isEmpty() && !portIsNumber)
    {
        m_databaseConnected = false;
        emit databaseStatusChanged("Port must be a number.");
        emit databaseConnectedChanged(false);
        return;
    }

    const bool connected =
        m_dataBaseManager->openRemoteDatabase(
            "proteus_online_database",
            driver,
            hostName.trimmed(),
            portIsNumber ? portNumber : 0,
            databaseName.trimmed(),
            userName.trimmed(),
            password);

    m_databaseConnected = connected;

    if (m_databaseConnected)
    {
        loadNormalizationState(
            driver
            + "|"
            + hostName.trimmed()
            + "|"
            + port.trimmed()
            + "|"
            + databaseName.trimmed());

        QSettings settings("DataBaseSettings", "Proteus");
        settings.setValue("database/isLocalConnection", false);
        settings.setValue("database/onlineDriver", driverName);
        settings.setValue("database/onlineDatabaseName", databaseName.trimmed());
        settings.setValue("database/onlineHostName", hostName.trimmed());
        settings.setValue("database/onlinePort", port.trimmed());
        settings.setValue("database/onlineUserName", userName.trimmed());

        emit databaseStatusChanged("Connected");
    }
    else
    {
        emit databaseStatusChanged(
            m_dataBaseManager->lastError().isEmpty()
                ? "Connection failed"
                : m_dataBaseManager->lastError());
    }

    emit databaseConnectedChanged(m_databaseConnected);
}

void AppController::onGenerateNormalization(
    const QString& form)
{
    if (!normalizationForms().contains(form))
    {
        m_normalizationStatus =
            "Unknown normalization form.";
        emit normalizationChanged();
        return;
    }

    if (!m_dataBaseManager->isConnected())
    {
        m_normalizationStatus =
            "Connect a database before normalization.";
        emit normalizationChanged();
        return;
    }

    if (m_dataBaseManager->getTableNames().isEmpty())
    {
        m_normalizationStatus =
            "The connected database does not contain any tables. Create or connect a schema before normalization.";
        m_normalizationBeforeSchema.clear();
        m_normalizationAfterSchema.clear();
        emit normalizationChanged();
        return;
    }

    const int existingVersion =
        normalizationVersionIndex(form);
    if (existingVersion >= 0)
    {
        if (existingVersion == m_activeNormalizationVersion)
        {
            m_selectedNormalizationForm = form;
            m_normalizationReady = false;
            m_pendingNormalizationVersion = -1;
            m_normalizationInputTables.clear();
            m_normalizationOutput.clear();
            m_normalizationBeforeSchema =
                m_dataBaseManager->buildSchemaDiagramForTables(
                    sourceNormalizationTables());
            m_normalizationAfterSchema =
                m_dataBaseManager->buildSchemaDiagramForTables(
                    activeNormalizationTables());
            m_normalizationStatus =
                form + " is already the active normalization version.";
            emit normalizationChanged();
            return;
        }

        prepareNormalizationVersion(
            existingVersion,
            "Version switch");
        return;
    }

    m_selectedNormalizationForm = form;
    m_normalizationOutput.clear();
    m_normalizationPrompt.clear();
    m_normalizationRepairAttempts = 0;
    m_normalizationReady = false;
    m_pendingNormalizationVersion = -1;
    m_normalizationInputTables =
        NormalizationPlanner::inputTables(
            form,
            m_appliedNormalizationForm,
            sourceNormalizationTables(),
            activeNormalizationTables());
    if (m_normalizationInputTables.isEmpty())
    {
        m_normalizationStatus =
            "No source tables are available for the selected normalization target.";
        emit normalizationChanged();
        return;
    }
    m_normalizationBeforeSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            sourceNormalizationTables());
    if (m_normalizationBeforeSchema.isEmpty())
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagram();
    m_normalizationAfterSchema.clear();
    m_normalizationStatus =
        "Generating a lossless "
        + form
        + " migration...";
    m_loading = true;
    emit loadingChanged();
    emit normalizationChanged();

    if (!m_aiEnvironmentReady)
    {
        m_loading = false;
        m_normalizationStatus =
            m_aiSetupInstructions;
        emit loadingChanged();
        emit normalizationChanged();
        return;
    }

    const QString driver =
        m_dataBaseManager->databaseDriver();
    const QString dialect =
        databaseDialectName(driver);

    QString prompt =
        "You are a professional database normalization and migration expert. "
        "Generate a migration for "
        + dialect
        + " that transforms the schema toward "
        + form
        + ". "
        + normalizationGoal(form)
        + " "
        + normalizationDialectRules(driver)
        + (m_appliedNormalizationForm.isEmpty()
               ? " The selected target is cumulative: apply every required transformation from 1NF through "
                     + form + ". "
               : " Continue from the active "
                     + m_appliedNormalizationForm
                     + " schema toward "
                     + form
                     + ". Do not recreate the previous version. ")
        + "Infer the natural language and naming style independently from each source table and its columns. "
          "Name generated tables, columns, constraints and indexes in that same language and style. "
          "Never translate German identifiers to English or English identifiers to German. "
          "Use the supplied sample rows and delimiter profiles to detect repeating groups and aligned list values. "
          "A delimiter alone is not proof of a repeating group; compare column meaning and equal token counts across rows. "
          "When multiple columns contain aligned lists, split them by ordinal into atomic child rows without changing their pairing. "
          "Infer functional dependencies only when supported by keys, relationships, identifier meaning, repeated values or samples. "
          "Preserve every original atomic value in the normalized target tables. "
          "Only normalize the input schema tables supplied below. Ignore retained versions that are not listed. "
          "Existing data must never be deleted, overwritten or truncated. "
          "Keep every existing source table and every existing row unchanged. "
          "Create new normalized tables with clear names and copy data using INSERT INTO ... SELECT. "
          "Do not CREATE a table with a name that already exists in the input schema. "
          "Use SELECT DISTINCT only when it does not remove semantically different rows. "
          "Create primary keys, unique constraints, foreign keys and indexes where justified by the schema. "
          "Do not claim that normalization is complete when sample rows still contain supported repeating groups. "
          "Return NO_CHANGES_REQUIRED only when the schema and supplied data evidence already satisfy the selected form. "
          "NO_CHANGES_REQUIRED is forbidden when the analysis reports denormalized table names, repeated numbered column groups, aligned list values or embedded dependent entities. "
          "If a dependency is uncertain, preserve the involved values in a lossless child relation instead of discarding or merging them. "
          "Allowed statements are CREATE TABLE, CREATE INDEX, INSERT INTO ... SELECT, common table expressions used by INSERT INTO ... SELECT, and safe ALTER TABLE ADD or RENAME. "
          "Never output DROP, DELETE, TRUNCATE, UPDATE, REPLACE, ATTACH, DETACH or VACUUM. "
          "Do not output BEGIN, COMMIT or ROLLBACK because the application manages the transaction. "
          "Return raw executable SQL only, without markdown, code fences, comments or explanations.\n\n";

    prompt +=
        m_dataBaseManager->buildNormalizationAnalysis(
            m_normalizationInputTables);

    m_normalizationPrompt = prompt;

    m_ollamaClient->generate(
        m_selectedModel,
        m_normalizationPrompt,
        OllamaClient::GenerateType::Normalization);
}

QString AppController::onApplyNormalization()
{
    if (m_pendingNormalizationVersion >= 0)
    {
        m_activeNormalizationVersion =
            m_pendingNormalizationVersion;
        m_pendingNormalizationVersion = -1;

        const NormalizationVersion& activeVersion =
            m_normalizationHistory.at(
                m_activeNormalizationVersion);
        m_appliedNormalizationForm = activeVersion.form;
        m_lastAppliedNormalizationSql =
            activeVersion.migrationSql;
        m_normalizationReady = false;
        m_selectedNormalizationForm = activeVersion.form;
        m_normalizationOutput.clear();
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagramForTables(
                sourceNormalizationTables());
        m_normalizationAfterSchema =
            activeVersion.form.isEmpty()
                ? QVariantList{}
                : m_dataBaseManager->buildSchemaDiagramForTables(
                      activeVersion.tableNames);
        m_normalizationStatus =
            activeVersion.form.isEmpty()
                ? "Original schema version reactivated. No data was deleted."
                : activeVersion.form
                      + " schema version reactivated. No data was deleted.";
        saveNormalizationState();
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    if (m_normalizationOutput.compare(
            "NO_CHANGES_REQUIRED",
            Qt::CaseInsensitive) == 0)
    {
        const NormalizationVersion currentVersion =
            m_normalizationHistory.at(
                m_activeNormalizationVersion);
        m_activeNormalizationVersion =
            storeNormalizationVersion({
                m_selectedNormalizationForm,
                currentVersion.migrationSql,
                currentVersion.tableNames
            });
        m_appliedNormalizationForm = m_selectedNormalizationForm;
        m_lastAppliedNormalizationSql =
            currentVersion.migrationSql;
        m_normalizationReady = false;
        m_normalizationBeforeSchema =
            m_dataBaseManager->buildSchemaDiagramForTables(
                sourceNormalizationTables());
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramForTables(
                currentVersion.tableNames);
        m_normalizationStatus =
            "Schema version registered as "
            + m_appliedNormalizationForm
            + ". No SQL execution was required.";
        saveNormalizationState();
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    if (!m_normalizationReady)
    {
        m_normalizationStatus =
            "No validated migration is ready to apply.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    if (!m_dataBaseManager->executeMigration(
            m_normalizationOutput))
    {
        m_normalizationStatus =
            "Migration rolled back: "
            + m_dataBaseManager->lastError();
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    m_appliedNormalizationForm =
        m_selectedNormalizationForm;
    m_lastAppliedNormalizationSql =
        m_normalizationOutput;
    QStringList targetTables =
        m_dataBaseManager->migrationTargetTableNames(
            m_lastAppliedNormalizationSql);
    if (targetTables.isEmpty())
        targetTables = activeNormalizationTables();
    m_activeNormalizationVersion =
        storeNormalizationVersion({
            m_appliedNormalizationForm,
            m_lastAppliedNormalizationSql,
            targetTables
        });
    m_pendingNormalizationVersion = -1;
    m_normalizationReady = false;
    m_normalizationBeforeSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            sourceNormalizationTables());
    m_normalizationAfterSchema =
        m_dataBaseManager->buildSchemaDiagramForTables(
            targetTables);
    if (m_normalizationAfterSchema.isEmpty())
    {
        m_normalizationAfterSchema =
            m_dataBaseManager->buildSchemaDiagramWithMigration(
                m_lastAppliedNormalizationSql);
    }
    m_normalizationStatus =
        m_appliedNormalizationForm
        + " applied successfully. Existing source data was retained.";

    saveNormalizationState();
    emit normalizationChanged();
    return m_normalizationStatus;
}

QString AppController::onResetNormalization()
{
    if (m_loading)
    {
        m_normalizationStatus =
            "Wait until the current normalization request is finished.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    if (!m_normalizationReady
        && m_pendingNormalizationVersion < 0
        && !m_selectedNormalizationForm.isEmpty()
        && m_selectedNormalizationForm.compare(
               m_appliedNormalizationForm,
               Qt::CaseInsensitive) != 0)
    {
        clearNormalizationPreview(
            m_appliedNormalizationForm.isEmpty()
                ? "Normalization selection cleared. Original schema is active."
                : "Normalization selection cleared. Active version remains "
                      + m_appliedNormalizationForm
                      + ".");
        return m_normalizationStatus;
    }

    if (m_normalizationReady
        && m_pendingNormalizationVersion < 0)
    {
        clearNormalizationPreview(
            "Normalization preview cleared. No database changes were applied.");
        return m_normalizationStatus;
    }

    const int baseVersion =
        m_pendingNormalizationVersion >= 0
            ? m_pendingNormalizationVersion
            : m_activeNormalizationVersion;
    if (baseVersion < 0
        || baseVersion >= m_normalizationHistory.size())
    {
        m_normalizationStatus =
            "No earlier normalization version is available.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    const QString baseForm =
        m_normalizationHistory.at(baseVersion).form;
    const int baseRank =
        NormalizationPlanner::formRank(baseForm);
    if (baseRank < 0)
    {
        m_normalizationStatus =
            "Original schema is already the earliest available version.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    const QString previousForm =
        baseRank == 0
            ? QString()
            : NormalizationPlanner::previousForm(baseForm);
    const int previousVersion =
        normalizationVersionIndex(previousForm);
    if (previousVersion >= 0)
    {
        prepareNormalizationVersion(
            previousVersion,
            "Previous level");
    }
    else
    {
        onGenerateNormalization(previousForm);
    }
    return m_normalizationStatus;
}

QString AppController::onAdvanceNormalization()
{
    if (m_loading)
    {
        m_normalizationStatus =
            "Wait until the current normalization request is finished.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    const int baseVersion =
        m_pendingNormalizationVersion >= 0
            ? m_pendingNormalizationVersion
            : m_activeNormalizationVersion;
    if (baseVersion < 0
        || baseVersion >= m_normalizationHistory.size())
    {
        m_normalizationStatus =
            "No active normalization version is available.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    const QString nextForm =
        NormalizationPlanner::nextForm(
            m_normalizationHistory.at(baseVersion).form);
    if (nextForm.isEmpty())
    {
        m_normalizationStatus =
            "The active schema is already at the last supported normal form.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    const int nextVersion =
        normalizationVersionIndex(nextForm);
    if (nextVersion >= 0)
    {
        prepareNormalizationVersion(
            nextVersion,
            "Next level");
    }
    else
    {
        onGenerateNormalization(nextForm);
    }
    return m_normalizationStatus;
}

QStringList AppController::codeLanguages() const
{
    return {
        "C++",
        "C",
        "C#",
        "Python",
        "Go",
        "Rust",
        "F#",
        "PowerShell",
        "Java"
    };
}

QStringList AppController::databaseDriverNames() const
{
    return {
        "MySQL / MariaDB",
        "PostgreSQL",
        "SQL Server / ODBC"
    };
}

QStringList AppController::normalizationForms() const
{
    return NormalizationPlanner::forms();
}

QString AppController::selectedLanguageName() const
{
    QStringList languages = codeLanguages();
    int index = static_cast<int>(m_selectedLanguageType);

    if (index < 0 || index >= languages.size())
        return "Code";

    return languages.at(index);
}

bool AppController::executable() const
{
    return m_isExecutable;
}

bool AppController::ollamaInstalled() const
{
    return m_ollamaInstalled;
}

bool AppController::ollamaRunning() const
{
    return m_ollamaRunning;
}

bool AppController::aiEnvironmentReady() const
{
    return m_aiEnvironmentReady;
}

QString AppController::normalizationOutput() const
{
    return m_normalizationOutput;
}

QString AppController::normalizationStatus() const
{
    return m_normalizationStatus;
}

QString AppController::selectedNormalizationForm() const
{
    return m_selectedNormalizationForm;
}

QString AppController::appliedNormalizationForm() const
{
    return m_appliedNormalizationForm;
}

bool AppController::normalizationReady() const
{
    return m_normalizationReady;
}

bool AppController::canResetNormalization() const
{
    return !m_loading
           && (m_pendingNormalizationVersion >= 0
               || m_normalizationReady
               || !m_selectedNormalizationForm.isEmpty()
               || m_activeNormalizationVersion > 0);
}

bool AppController::canAdvanceNormalization() const
{
    const int baseVersion =
        m_pendingNormalizationVersion >= 0
            ? m_pendingNormalizationVersion
            : m_activeNormalizationVersion;

    if (m_loading
        || (m_normalizationReady
            && m_pendingNormalizationVersion < 0)
        || baseVersion < 0
        || baseVersion >= m_normalizationHistory.size())
    {
        return false;
    }

    return !NormalizationPlanner::nextForm(
                m_normalizationHistory.at(baseVersion).form)
                .isEmpty();
}

QVariantList AppController::normalizationBeforeSchema() const
{
    return m_normalizationBeforeSchema;
}

QVariantList AppController::normalizationAfterSchema() const
{
    return m_normalizationAfterSchema;
}

QVariantMap AppController::codeGenerationOptions() const
{
    return m_codeGenerationOptions;
}

QVariantMap AppController::codeGenerationCapabilities() const
{
    return CodeGenerationProfile::capabilities(
        m_selectedLanguageType);
}

bool AppController::generatedCodeValid() const
{
    return m_generatedCodeValid;
}

QString AppController::codeGenerationValidationSummary() const
{
    return m_codeGenerationValidationSummary;
}

QVariantList AppController::codeAssistantMessages() const
{
    return m_codeAssistantMessages;
}

bool AppController::codeAssistantBusy() const
{
    return m_codeAssistantBusy;
}

void AppController::setCodeGenerationOptions(
    const QVariantMap& options)
{
    const QVariantMap normalizedOptions =
        CodeGenerationProfile::optionsFor(
            m_selectedLanguageType,
            options)
            .toVariantMap();

    if (m_codeGenerationOptions == normalizedOptions)
        return;

    m_codeGenerationOptions = normalizedOptions;

    QSettings settings("DataBaseSettings", "Proteus");
    settings.setValue(
        codeGenerationSettingsKey(m_selectedLanguageType),
        m_codeGenerationOptions);
    settings.setValue(
        "codeGeneration/options",
        m_codeGenerationOptions);

    emit codeGenerationSettingsChanged();
}

void AppController::setSelectedLanguage(int index)
{
    if (index < 0 || index >= codeLanguages().size())
        return;

    const auto language =
        static_cast<ProgrammingLanguage::ProgrammingLanguageType>(index);

    if (m_selectedLanguageType == language)
        return;

    m_selectedLanguageType =
        language;

    QSettings settings("DataBaseSettings", "Proteus");
    m_codeGenerationOptions =
        CodeGenerationProfile::optionsFor(
            m_selectedLanguageType,
            settings.value(
                codeGenerationSettingsKey(m_selectedLanguageType))
                .toMap())
            .toVariantMap();

    m_codeAssistantMessages.clear();
    m_codeAssistantBusy = false;

    m_generatedCodeValid = false;
    m_codeGenerationValidationSummary =
        "Generate code for the selected language to run validation.";

    emit languageChanged();
    emit codeGenerationSettingsChanged();
    emit generatedCodeValidationChanged();
    emit codeAssistantChanged();
}

void AppController::setSelectedModel(const QString& model)
{
    if (m_selectedModel == model)
        return;

    m_selectedModel = model;
    emit selectedModelChanged();
}

void AppController::setOllamaEndpoint(const QString& endpoint)
{
    const QString trimmedEndpoint =
        endpoint.trimmed();

    if (m_ollamaEndpoint == trimmedEndpoint)
        return;

    m_ollamaEndpoint = trimmedEndpoint;

    QSettings settings("DataBaseSettings", "Proteus");
    settings.setValue("ai/ollamaEndpoint", trimmedEndpoint);

    emit ollamaEndpointChanged();
    refreshAiEnvironment();
}

void AppController::setIsLocalDatabase(bool isLocal)
{
    if (m_isLocalDatabase == isLocal)
        return;

    m_isLocalDatabase = isLocal;

    QSettings settings("DataBaseSettings", "Proteus");
    settings.setValue("database/isLocalConnection", isLocal);

    emit isLocalDatabaseChanged();
}

void AppController::setDalOutputPath(const QString& path)
{
    if (m_dalOutputPath == path)
        return;

    m_dalOutputPath = path;

    QSettings settings("DataBaseSettings", "Proteus");
    settings.setValue("dal/outputPath", path);

    emit dalOutputPathChanged();
}

void AppController::setClassesFolderPath(const QString& path)
{
    if (m_classFolderPath == path)
        return;

    m_classFolderPath = path;

    QSettings settings("DataBaseSettings", "Proteus");
    settings.setValue("classes/scripts", path);

    emit classesFolderPathChanged();
}

void AppController::setAddAuditFields(bool enabled)
{
    m_addAuditFields = enabled;
}

void AppController::onGenerateSqlCode()
{
    m_loading = true;
    emit loadingChanged();

    if (m_classFolderPath.isEmpty())
    {
        emit warningOccurred(
            "Missing path",
            "Please select a class folder first."
            );

        m_loading = false;
        emit loadingChanged();
        return;
    }

    if (!m_aiEnvironmentReady)
    {
        emit warningOccurred(
            "AI environment",
            m_aiSetupInstructions
            );

        m_loading = false;
        emit loadingChanged();
        return;
    }

    if (m_selectedModel.isEmpty())
    {
        emit warningOccurred(
            "Missing AI model",
            "Please select an AI model first."
            );

        m_loading = false;
        emit loadingChanged();
        return;
    }

    if (!m_dataBaseManager->isConnected())
    {
        emit warningOccurred(
            "Database",
            "Please connect to a database first."
            );

        m_loading = false;
        emit loadingChanged();
        return;
    }

    ClassScanner scanner;
    ClassParser parser;

    emit sqlOutputChanged("Scanning class files...\n");

    QList<ScannedClassFile> files =
        scanner.scanAndReadClassFiles(
            m_classFolderPath,
            m_selectedLanguageType
            );

    if (files.isEmpty())
    {
        emit warningOccurred(
            "No files found",
            "No class files were found in the selected folder."
            );

        m_loading = false;
        emit loadingChanged();
        return;
    }

    QList<ParsedClass> databaseClasses;

    for (const ScannedClassFile& file : files)
    {
        databaseClasses.append(
            parser.parseDatabaseClasses(
                file.content,
                m_selectedLanguageType));
    }

    if (databaseClasses.isEmpty())
    {
        emit warningOccurred(
            "No database classes found",
            "The selected folder contains no supported class or struct declarations with data fields. Object instances and unrelated source files cannot be converted to a SQL schema."
            );

        m_loading = false;
        emit loadingChanged();
        return;
    }

    const QString databaseDialect =
        databaseDialectName(
            m_dataBaseManager->databaseDriver());

    m_prompt =
        "You are a professional "
        + databaseDialect
        + " database architect. "
        "Generate SQL that is fully compatible with "
        + databaseDialect
        + ". "
        "Analyze all provided classes, attributes and database metadata. "

        "Use only the provided class names, attribute names and database information. "
        "Never rename classes. "
        "Never rename attributes. "
        "Never replace an existing attribute with a different name. "
        "Attribute names must remain exactly as provided. "
        "If an attribute is named username, it must remain username. "
        "Do not convert username to name or any other synonym. "
        "Do not invent attributes that are not provided. "
        "Respect exact attribute names, spelling and casing. "

        "If a table does not exist, generate a CREATE TABLE statement. "
        "If a table already exists, do not recreate it. "
        "If an attribute is missing in an existing table, generate ALTER TABLE ADD COLUMN. "
        "If an attribute already exists, do not generate it again. "

        "Never drop, delete, recreate or overwrite existing tables. "
        "Never remove existing columns. "
        "Never destroy existing data. "
        "No Duplicate Columns: Never generate ALTER TABLE to add a new column if a field with the same logical purpose already exists. "
        "Semantic Mapping: Map fields by their meaning, not just exact names. If a matching concept exists under a slightly different name, reuse it instead of creating a new column. "
        "Every detected class and attribute must be considered exactly as provided. "
        "Use datatypes that are appropriate for the connected database. "
        "Create primary keys and foreign keys where appropriate. "
        "Use automatic identifier generation syntax only when it is supported by the connected database and logically required. ";

    if (m_addAuditFields)
    {
        m_prompt +=
            "Add createdAt and updatedAt columns to all generated tables. ";
    }
    else
    {
        m_prompt +=
            "Do not add createdAt or updatedAt unless they are explicitly defined in the provided classes. ";
    }

    m_prompt +=
        "Return only executable SQL statements for "
        + databaseDialect
        + ". "
        "No markdown. "
        "No code fences. "
        "Do not wrap SQL in ```sql blocks. "
        "Return raw SQL only. "
        "No comments. "
        "No explanation.\n\n";

    emit sqlOutputChanged(
        QString("Found %1 database class(es).\n")
            .arg(databaseClasses.size())
        + "Parsing classes and attributes...\n"
        "Checking existing database tables...\n"
        "Checking existing columns...\n"
        );

    for (const ParsedClass& cls : databaseClasses)
    {
        m_prompt += "Class: " + cls.name + "\n";

        bool tableExists =
            m_dataBaseManager->tableExists(cls.name);

        if (tableExists)
        {
            m_prompt += "Existing table: yes\n";

            if (m_dataBaseManager->hasRows(cls.name))
                m_prompt += "Table contains data: yes\n";
            else
                m_prompt += "Table contains data: no\n";

            QStringList existingColumns =
                m_dataBaseManager->getColumnNames(cls.name);

            m_prompt +=
                "Existing columns: "
                + existingColumns.join(", ")
                + "\n";
        }
        else
        {
            m_prompt += "Existing table: no\n";
        }

        for (const ParsedAttribute& attribute : cls.attributes)
        {
            const QString sqlType =
                ProgrammingLanguage::mapToSqlType(
                    attribute.type,
                    m_selectedLanguageType);

            m_prompt += "- "
                        + attribute.type
                        + " "
                        + attribute.name
                        + " sql_type="
                        + sqlType;

            if (ProgrammingLanguage::isNullableType(attribute.type))
                m_prompt += " nullable";

            if (ProgrammingLanguage::isCollectionType(attribute.type))
                m_prompt += " collection";

            if (tableExists)
            {
                if (m_dataBaseManager->columnExists(
                        cls.name,
                        attribute.name))
                {
                    m_prompt += " existing_column";
                }
                else
                {
                    m_prompt += " missing_column";
                }
            }

            if (attribute.isRelation)
                m_prompt += " relationship";

            m_prompt += "\n";
        }

        m_prompt += "\n";
    }

    emit sqlOutputChanged("Generating SQL schema with AI...");

    m_ollamaClient->generate(
        m_selectedModel,
        m_prompt,
        OllamaClient::GenerateType::Sql
        );
}

void AppController::onGenerateDalCode(bool secureAccess)
{
    Q_UNUSED(secureAccess);

    onGenerateApplicationCode(
        m_codeGenerationOptions);
}

void AppController::onGenerateApplicationCode(
    const QVariantMap& optionValues)
{
    const CodeGenerationOptions options =
        CodeGenerationProfile::optionsFor(
            m_selectedLanguageType,
            optionValues);

    if (options.requestedLayers().isEmpty())
    {
        emit dalStatusChanged(
            "Select at least one code layer before generation.");
        return;
    }

    setCodeGenerationOptions(
        options.toVariantMap());

    m_loading = true;
    m_generatedCodeValid = false;
    m_codeGenerationValidationSummary =
        "Waiting for generated code.";
    emit loadingChanged();
    emit generatedCodeValidationChanged();

    if (!m_aiEnvironmentReady)
    {
        m_loading = false;
        emit loadingChanged();
        emit dalStatusChanged(
            m_aiSetupInstructions
            );
        return;
    }

    if (!m_dataBaseManager->isConnected())
    {
        m_loading = false;
        emit loadingChanged();
        emit dalStatusChanged(
            "No database connection available."
            );
        return;
    }

    m_lastCodeGenerationTables =
        activeNormalizationTables();
    if (m_lastCodeGenerationTables.isEmpty())
    {
        m_lastCodeGenerationTables =
            m_dataBaseManager->getTableNames();
    }

    if (m_lastCodeGenerationTables.isEmpty())
    {
        m_loading = false;
        emit loadingChanged();
        emit dalStatusChanged(
            "The connected database does not contain tables to generate code for.");
        return;
    }

    const QString databaseDialect =
        databaseDialectName(
            m_dataBaseManager->databaseDriver());

    m_generationLanguageType =
        m_selectedLanguageType;
    m_codeGenerationRepairAttempts = 0;

    QString dalPrompt =
        "You are a professional application architecture and secure database code generator. "
        "Generate a complete, internally consistent file set for the following "
        + databaseDialect
        + " schema. ";

    emit dalStatusChanged(
        CodeGenerationProfile::generationStatus(
            m_generationLanguageType,
            options));

    dalPrompt +=
        "Use real table names and real column names exactly as provided. "
        "Use the mandatory naming convention Sql<TableName> only for generated data access implementations. "
        "For table User generate class SqlUser and files SqlUser.h and SqlUser.cpp. "
        "For table UserList generate SqlUserList. "
        "Never append Repository, DAO, DAL or Manager to Sql<TableName> implementations. "
        "Keep the original table name unchanged inside SQL statements. "
        "Never use placeholder names like ExampleRepository. "
        "Never output placeholder tags like <header code> or <source code>. "
        "Generate complete imports, constructors, types, error handling and method bodies. "
        "Do not omit code with comments such as implementation omitted. "
        "Data access implementations must provide create, read, update and delete operations. ";

    dalPrompt +=
        CodeGenerationProfile::buildPromptInstructions(
            m_generationLanguageType,
            databaseDialect,
            options);

    if (!m_appliedNormalizationForm.isEmpty())
    {
        dalPrompt +=
            "The database has been normalized toward "
            + m_appliedNormalizationForm
            + ". Prefer the normalized destination tables created by the applied migration. "
              "Treat retained source tables as legacy migration sources when equivalent normalized destination tables exist. "
              "Generate coordinated DAL methods for foreign-key relationships and use explicit JOIN statements for composed reads. "
              "Use a database transaction when one logical write spans multiple normalized tables. "
              "Keep all query values parameterized and never concatenate identifiers or values from user input. ";

        if (!m_lastAppliedNormalizationSql.isEmpty())
        {
            dalPrompt +=
                "The applied lossless migration was:\n"
                + m_lastAppliedNormalizationSql
                + "\nEnd of applied migration. ";
        }
    }

    dalPrompt +=
        "Generate every requested layer for every listed table. "
        "Return each file exactly in this format:\n"
        "FILE: ExactFileName.ext\n"
        "complete file content\n\n"
        "No markdown. No explanation. No code fences.\n\n";

    dalPrompt +=
        m_dataBaseManager->buildSchemaDescription(
            m_lastCodeGenerationTables);

    m_lastCodeGenerationPrompt =
        dalPrompt;

    m_ollamaClient->generate(
        m_selectedModel,
        dalPrompt,
        OllamaClient::GenerateType::Dal
        );
}

bool AppController::validateGeneratedCode(
    const QString& response)
{
    const QStringList validationErrors =
        CodeGenerationProfile::validateResponse(
            response,
            m_generationLanguageType,
            CodeGenerationProfile::optionsFor(
                m_generationLanguageType,
                m_codeGenerationOptions),
            m_lastCodeGenerationTables);

    m_generatedCodeValid =
        validationErrors.isEmpty();
    m_codeGenerationValidationSummary =
        m_generatedCodeValid
            ? "Generated code passed file, layer and parameter-binding validation."
            : "Validation failed:\n- "
                  + validationErrors.join("\n- ");

    emit generatedCodeValidationChanged();
    return m_generatedCodeValid;
}

void AppController::askCodeAssistant(
    const QString& question,
    const QString& generatedCode)
{
    const QString trimmedQuestion = question.trimmed();
    if (trimmedQuestion.isEmpty() || m_codeAssistantBusy)
        return;

    m_codeAssistantMessages.append(QVariantMap{
        {"role", "user"},
        {"text", trimmedQuestion}
    });

    if (!m_aiEnvironmentReady)
    {
        m_codeAssistantMessages.append(QVariantMap{
            {"role", "assistant"},
            {"text", m_aiSetupInstructions}
        });
        emit codeAssistantChanged();
        return;
    }

    const CodeGenerationOptions options =
        CodeGenerationProfile::optionsFor(
            m_selectedLanguageType,
            m_codeGenerationOptions);
    const QString dialect =
        m_dataBaseManager->isConnected()
            ? databaseDialectName(
                  m_dataBaseManager->databaseDriver())
            : "no connected database";
    const QString schema =
        m_dataBaseManager->isConnected()
            ? m_dataBaseManager->buildSchemaDescription(
                  activeNormalizationTables())
            : "No database schema is connected.";
    const QString codeExcerpt =
        generatedCode.trimmed().isEmpty()
            ? "No code has been generated yet."
            : generatedCode.left(16000);

    QString prompt =
        "You are the ProteusManager project code assistant. "
        "Answer the user's concrete architecture or implementation question for the current project. "
        "Be concise, distinguish recommendations from requirements, and never claim generated code is guaranteed to compile. "
        "Keep all database examples parameterized and never suggest concatenating user input into SQL. "
        "Current language: "
        + selectedLanguageName()
        + ". Current database: "
        + dialect
        + ". Current generation settings: architecture="
        + options.architecture
        + ", databaseApi="
        + options.databaseApi
        + ", dataAccessPattern="
        + options.dataAccessPattern
        + ", layers="
        + options.requestedLayers().join(", ")
        + ".\n\nSchema:\n"
        + schema
        + "\n\nGenerated code excerpt:\n"
        + codeExcerpt
        + "\n\nUser question:\n"
        + trimmedQuestion;

    m_codeAssistantBusy = true;
    emit codeAssistantChanged();
    m_ollamaClient->generate(
        m_selectedModel,
        prompt,
        OllamaClient::GenerateType::Assistant);
}

void AppController::clearCodeAssistant()
{
    if (m_codeAssistantBusy || m_codeAssistantMessages.isEmpty())
        return;

    m_codeAssistantMessages.clear();
    emit codeAssistantChanged();
}

void AppController::onExportDalCode(const QString& response, const QString& outputPath)
{
    if (!validateGeneratedCode(response))
    {
        emit dalExportFinished(
            "Export blocked because generated code validation failed.");
        return;
    }

    m_loading = true;
    emit loadingChanged();

    const DalExportResult result =
        DalFileExporter::exportFiles(
            response,
            outputPath);

    m_loading = false;
    emit loadingChanged();

    emit dalExportFinished(
        result.message
        );
}

QString AppController::onExecuteSqlCode(const QString& response)
{
    m_loading = true;
    emit loadingChanged();

    if (!m_dataBaseManager->isValidSql(response))
    {
        m_isExecutable = false;
        emit executableChanged();
        m_loading = false;
        emit loadingChanged();

        return "Invalid SQL\nThe AI response does not contain valid SQL.";
    }

    if (m_dataBaseManager->executeQuery(response))
    {
        m_isExecutable = false;
        emit executableChanged();
        m_loading = false;
        emit loadingChanged();
        return response;
    }

    m_loading = false;
    emit loadingChanged();

    return "SQL execution failed.";
}
