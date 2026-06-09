#include "appcontroller.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_ollamaClient(new OllamaClient(this))
    , m_dataBaseManager(std::make_unique<DatabaseManager>())
{
    QSettings settings("DataBaseSettings", "Proteus");

    m_isLocalDatabase = settings.value("database/isLocalConnection", true).toBool();
    m_dalOutputPath = settings.value("dal/outputPath","").toString();
    m_classFolderPath = settings.value("classes/scripts", "").toString();
   // emit dalOutputPathChanged(m_classPath);
    emit dalOutputPathChanged();
    emit classesFolderPathChanged();

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
            emit modelsFetched(models);

            if (!models.isEmpty())
            {
                m_selectedModel = models.first();
                emit selectedModelChanged();
            }
        });

    connect(
        m_ollamaClient,
        &OllamaClient::dalReceived,
        this,
        [this](const QString& code)
        {
            emit dalOutputChanged(code);
            QSettings settings("DataBaseSettings", "Proteus");
            m_dalOutputPath = settings.value("dal/outputPath").toString();
            emit dalOutputPathChanged();
            m_isExecutable = true;
            emit executableChanged();
            m_loading = false;
            emit loadingChanged();
        });

    fetchModels();
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

bool AppController::isLocalDatabase() const
{
    return m_isLocalDatabase;
}

void AppController::fetchModels()
{
    m_ollamaClient->fetchModels();
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
        emit databaseStatusChanged("Connected");
    }
    else
    {
        emit databaseStatusChanged("Connection failed");
    }

    emit databaseConnectedChanged(m_databaseConnected);
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

void AppController::setSelectedLanguage(int index)
{
    m_selectedLanguageType =
        static_cast<ProgrammingLanguage::ProgrammingLanguageType>(index);

    emit languageChanged();
}

void AppController::setSelectedModel(const QString& model)
{
    if (m_selectedModel == model)
        return;

    m_selectedModel = model;
    emit selectedModelChanged();
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

    m_prompt =
        "You are a professional SQLite database architect. "
        "Generate SQL that is fully compatible with SQLite 3. "
        "Analyze all provided classes, attributes and database metadata. "
        "Never generate MySQL, PostgreSQL, SQL Server or Oracle syntax. "

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
        "Use appropriate SQLite datatypes. "
        "Create primary keys and foreign keys where appropriate. "
        "Use AUTOINCREMENT only when automatic identifier generation is logically required. ";

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
        "Return only executable SQLite SQL statements. "
        "No markdown. "
        "No code fences. "
        "Do not wrap SQL in ```sql blocks. "
        "Return raw SQL only. "
        "No comments. "
        "No explanation.\n\n";

    emit sqlOutputChanged(
        "Parsing classes and attributes...\n"
        "Checking existing database tables...\n"
        "Checking existing columns...\n"
        );

    for (const ScannedClassFile& file : files)
    {
        QList<ParsedClass> classes =
            parser.parseClasses(
                file.content,
                m_selectedLanguageType
                );

        for (const ParsedClass& cls : classes)
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
                m_prompt += "- "
                            + attribute.type
                            + " "
                            + attribute.name;

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
    m_loading = true;
    emit loadingChanged();
    if (!m_dataBaseManager->isConnected())
    {
        m_loading = false;
        emit loadingChanged();
        emit dalStatusChanged(
            "No database connection available."
            );
        return;
    }

    QString dalPrompt =
        "You are a professional database access layer generator. ";

    if (secureAccess)
    {
        dalPrompt +=
            "Generate secure database access classes for the following SQLite schema. ";
        emit dalStatusChanged(
            "Generating secure database access layer..."
            );
    }
    else
    {
        dalPrompt +=
            "Generate database access classes for the following SQLite schema. ";
        emit dalStatusChanged(
            "Generating database access layer..."
            );
    }

    dalPrompt +=
        "Use real table names and real column names exactly as provided. "
        "Never use placeholder names like ExampleRepository. "
        "Never output placeholder tags like <header code> or <source code>. "
        "Use prepared statements or parameter binding for all values. "
        "Never concatenate user input into SQL strings. "
        "Generate classes with create, read, update and delete methods. ";

    if (m_selectedLanguageType ==
        ProgrammingLanguage::ProgrammingLanguageType::Cplusplus)
    {
        emit dalStatusChanged("Generating .h and .cpp files...");

        dalPrompt +=
            "Target language: C++ with Qt. "
            "Use QSqlDatabase and QSqlQuery. "
            "Generate one .h file and one .cpp file per database table. ";
    }
    else if (m_selectedLanguageType ==
             ProgrammingLanguage::ProgrammingLanguageType::Csharp)
    {
        emit dalStatusChanged("Generating .cs files...");

        dalPrompt +=
            "Target language: C# with SQLite. "
            "Generate one .cs file per database table. "
            "Use parameterized queries. ";
    }

    dalPrompt +=
        "Return files exactly in this format:\n"
        "FILE: RealTableName.ext\n"
        "actual code\n\n"
        "No markdown. No explanation. No code fences.\n\n";

    dalPrompt +=
        m_dataBaseManager->buildSchemaDescription();

    m_ollamaClient->generate(
        m_selectedModel,
        dalPrompt,
        OllamaClient::GenerateType::Dal
        );
}

void AppController::onExportDalCode(const QString& response, const QString& outputPath)
{
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
