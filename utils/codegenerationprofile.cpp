#include "codegenerationprofile.h"

#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>

namespace
{
QString canonicalName(QString value)
{
    value = value.toLower();
    value.remove(QRegularExpression("[^a-z0-9]"));
    return value;
}

QString normalizedChoice(
    const QString& value,
    const QStringList& choices,
    const QString& fallback)
{
    for (const QString& choice : choices)
    {
        if (value.compare(choice, Qt::CaseInsensitive) == 0)
            return choice;
    }

    return fallback;
}

QString providerFor(
    ProgrammingLanguage::ProgrammingLanguageType language,
    const QString& databaseDialect)
{
    const bool sqlite =
        databaseDialect.contains("sqlite", Qt::CaseInsensitive);
    const bool mysql =
        databaseDialect.contains("mysql", Qt::CaseInsensitive)
        || databaseDialect.contains("mariadb", Qt::CaseInsensitive);
    const bool postgres =
        databaseDialect.contains("postgres", Qt::CaseInsensitive);

    using Type = ProgrammingLanguage::ProgrammingLanguageType;

    switch (language)
    {
    case Type::Cplusplus:
        return "Qt SQL through QSqlDatabase and QSqlQuery";
    case Type::C:
        if (sqlite)
            return "the SQLite C API with sqlite3 prepared statements";
        if (mysql)
            return "the MySQL C API prepared-statement interface";
        if (postgres)
            return "libpq with PQexecParams or prepared statements";
        return "the ODBC C API with SQLPrepare and SQLBindParameter";
    case Type::Csharp:
        if (sqlite)
            return "Microsoft.Data.Sqlite";
        if (mysql)
            return "MySqlConnector";
        if (postgres)
            return "Npgsql";
        return "Microsoft.Data.SqlClient";
    case Type::Python:
        if (sqlite)
            return "Python's sqlite3 module";
        if (mysql)
            return "mysql-connector-python";
        if (postgres)
            return "psycopg 3";
        return "pyodbc";
    case Type::Go:
        if (sqlite)
            return "database/sql with modernc.org/sqlite";
        if (mysql)
            return "database/sql with github.com/go-sql-driver/mysql";
        if (postgres)
            return "database/sql with github.com/jackc/pgx/v5/stdlib";
        return "database/sql with github.com/microsoft/go-mssqldb";
    case Type::Rust:
        if (sqlite || mysql || postgres)
            return "SQLx with the matching database feature";
        return "odbc-api for SQL Server through ODBC";
    case Type::Fsharp:
        if (sqlite)
            return "Microsoft.Data.Sqlite";
        if (mysql)
            return "MySqlConnector";
        if (postgres)
            return "Npgsql";
        return "Microsoft.Data.SqlClient";
    case Type::Powershell:
        if (sqlite)
            return "Microsoft.Data.Sqlite loaded as a .NET provider";
        if (mysql)
            return "MySqlConnector loaded as a .NET provider";
        if (postgres)
            return "Npgsql loaded as a .NET provider";
        return "Microsoft.Data.SqlClient";
    case Type::Java:
        return "JDBC with the official driver for " + databaseDialect;
    }

    return "the official database driver";
}

QString fileRules(
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    using Type = ProgrammingLanguage::ProgrammingLanguageType;

    switch (language)
    {
    case Type::Cplusplus:
        return "Generate matching .h and .cpp files. Use RAII and explicit ownership. ";
    case Type::C:
        return "Generate matching .h and .c files. Use opaque context structs and explicit cleanup functions. ";
    case Type::Csharp:
        return "Generate .cs files with namespaces and IDisposable/IAsyncDisposable where required. ";
    case Type::Python:
        return "Generate typed .py modules and use context managers for connections and transactions. ";
    case Type::Go:
        return "Generate .go files in a consistent package and pass context.Context to database operations. ";
    case Type::Rust:
        return "Generate .rs modules with Result-based error handling and no unwrap calls in database paths. ";
    case Type::Fsharp:
        return "Generate .fs files with modules or types in dependency-safe order. ";
    case Type::Powershell:
        return "Generate .ps1 files with advanced functions and terminating error handling. ";
    case Type::Java:
        return "Generate .java files with packages, try-with-resources and explicit checked error handling. ";
    }

    return {};
}

QString layerSuffix(const QString& layer)
{
    if (layer == "Repository / DAO")
        return "sql";
    if (layer == "Domain Model")
        return "model";
    return canonicalName(layer.section(' ', 0, 0));
}

bool fileMatchesLayer(
    const QString& baseName,
    const QString& tableName,
    const QString& layer)
{
    const QString fileKey = canonicalName(baseName);
    const QString tableKey = canonicalName(tableName.section('.', -1));

    if (!fileKey.contains(tableKey))
        return false;

    if (layer == "Repository / DAO")
    {
        return fileKey.startsWith("sql" + tableKey)
               || fileKey.contains(tableKey + "repository")
               || fileKey.contains(tableKey + "dao");
    }

    return fileKey.contains(layerSuffix(layer));
}

bool hasSecureBindingMarkers(
    const QString& response,
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    using Type = ProgrammingLanguage::ProgrammingLanguageType;

    switch (language)
    {
    case Type::Cplusplus:
        return response.contains(QRegularExpression("\\.prepare\\s*\\("))
               && response.contains("bindValue");
    case Type::C:
        return (response.contains("sqlite3_prepare")
                && response.contains("sqlite3_bind"))
               || response.contains("PQexecParams")
               || (response.contains("mysql_stmt_prepare")
                   && response.contains("mysql_stmt_bind_param"))
               || (response.contains("SQLPrepare")
                   && response.contains("SQLBindParameter"));
    case Type::Csharp:
    case Type::Fsharp:
    case Type::Powershell:
        return response.contains(
            QRegularExpression(
                R"((Parameters\s*\.\s*Add|DbParameter|SqlParameter|NpgsqlParameter|MySqlParameter))",
                QRegularExpression::CaseInsensitiveOption));
    case Type::Python:
        return response.contains(
            QRegularExpression(
                R"((execute|executemany)\s*\([^,\r\n]+,\s*[^)]+\))",
                QRegularExpression::CaseInsensitiveOption));
    case Type::Go:
        return response.contains(
            QRegularExpression(
                R"((Exec|Query|Prepare)(Context)?\s*\()"));
    case Type::Rust:
        return response.contains(".bind(")
               || response.contains("params![");
    case Type::Java:
        return response.contains("PreparedStatement")
               && response.contains(
                   QRegularExpression(R"(\.set[A-Z][A-Za-z]*\s*\()"));
    }

    return false;
}
}

CodeGenerationOptions CodeGenerationOptions::fromVariantMap(
    const QVariantMap& values)
{
    CodeGenerationOptions options;
    options.entity = values.value("entity", true).toBool();
    options.dto = values.value("dto", true).toBool();
    options.repository = values.value("repository", true).toBool();
    options.service = values.value("service", false).toBool();
    options.controller = values.value("controller", false).toBool();
    options.domainModel = values.value("domainModel", false).toBool();
    options.interfaces = values.value("interfaces", true).toBool();
    options.unitTests = values.value("unitTests", false).toBool();
    options.asyncOperations = values.value("asyncOperations", false).toBool();
    options.architecture = normalizedChoice(
        values.value("architecture", "Layered").toString(),
        {"Layered", "Clean Architecture", "Hexagonal"},
        "Layered");
    options.dataAccessPattern = normalizedChoice(
        values.value("dataAccessPattern", "Repository").toString(),
        {"Repository", "DAO"},
        "Repository");
    return options;
}

QVariantMap CodeGenerationOptions::toVariantMap() const
{
    return {
        {"entity", entity},
        {"dto", dto},
        {"repository", repository},
        {"service", service},
        {"controller", controller},
        {"domainModel", domainModel},
        {"interfaces", interfaces},
        {"unitTests", unitTests},
        {"asyncOperations", asyncOperations},
        {"architecture", architecture},
        {"dataAccessPattern", dataAccessPattern}
    };
}

QStringList CodeGenerationOptions::requestedLayers() const
{
    QStringList layers;
    if (entity)
        layers.append("Entity");
    if (dto)
        layers.append("DTO");
    if (repository)
        layers.append("Repository / DAO");
    if (service)
        layers.append("Service / BLL");
    if (controller)
        layers.append("Controller / Presentation");
    if (domainModel)
        layers.append("Domain Model");
    return layers;
}

QString CodeGenerationProfile::buildPromptInstructions(
    ProgrammingLanguage::ProgrammingLanguageType language,
    const QString& databaseDialect,
    const CodeGenerationOptions& options)
{
    const QString languageName =
        ProgrammingLanguage::languageTypeToText(language);
    const QStringList layers = options.requestedLayers();

    QString instructions =
        "Target language: " + languageName + ". "
        + "Database provider: " + providerFor(language, databaseDialect) + ". "
        + fileRules(language)
        + "Architecture: " + options.architecture + ". "
        + "Data access pattern: " + options.dataAccessPattern + ". "
        + "Generate only these layers: " + layers.join(", ") + ". "
        + "Keep dependencies pointing inward according to the selected architecture. "
          "Entities represent persisted rows and contain no database calls. "
          "DTOs transfer data between layers and contain no business or database logic. "
          "Domain models contain business behavior but no SQL or driver references. "
          "Services contain business workflows and depend on repository abstractions. "
          "Controllers validate input, call services and map results without issuing SQL. "
          "Only Repository / DAO files may contain SQL or database-driver calls. "
          "For every table, name the data access implementation Sql<TableName>. "
          "Name other generated types <TableName>Entity, <TableName>Dto, <TableName>Model, <TableName>Service and <TableName>Controller when requested. ";

    if (options.interfaces)
    {
        instructions +=
            "Generate interfaces or language-appropriate abstractions for repositories and services. ";
    }

    if (options.asyncOperations)
    {
        instructions +=
            "Use the language's supported asynchronous database APIs without blocking wrappers. ";
    }

    if (options.unitTests)
    {
        instructions +=
            "Generate unit tests for mapping and business logic and repository contract tests that do not require production credentials. ";
    }

    instructions +=
        "Use prepared statements and parameter binding for every value. "
        "Never concatenate or interpolate user input into SQL, identifiers, connection strings or commands. "
        "Use transactions for logical writes spanning multiple statements or tables. "
        "Do not hard-code credentials. "
        "Include FILE: dependencies.txt with exact external package names and minimum compatible versions. ";

    return instructions;
}

QString CodeGenerationProfile::generationStatus(
    ProgrammingLanguage::ProgrammingLanguageType language,
    const CodeGenerationOptions& options)
{
    return "Generating "
           + ProgrammingLanguage::languageTypeToText(language)
           + " code for "
           + options.requestedLayers().join(", ")
           + "...";
}

QStringList CodeGenerationProfile::allowedExtensions(
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    using Type = ProgrammingLanguage::ProgrammingLanguageType;

    switch (language)
    {
    case Type::Cplusplus:
        return {"h", "hpp", "cpp", "txt"};
    case Type::C:
        return {"h", "c", "txt"};
    case Type::Csharp:
        return {"cs", "txt"};
    case Type::Python:
        return {"py", "txt"};
    case Type::Go:
        return {"go", "txt"};
    case Type::Rust:
        return {"rs", "txt"};
    case Type::Fsharp:
        return {"fs", "txt"};
    case Type::Powershell:
        return {"ps1", "txt"};
    case Type::Java:
        return {"java", "txt"};
    }

    return {"txt"};
}

QStringList CodeGenerationProfile::validateResponse(
    const QString& response,
    ProgrammingLanguage::ProgrammingLanguageType language,
    const CodeGenerationOptions& options,
    const QStringList& tableNames)
{
    QStringList errors;

    if (response.trimmed().isEmpty())
        return {"The generated response is empty."};

    if (response.contains("```")
        || response.contains(
            QRegularExpression(
                R"(<[^>]*(code|implementation|content)[^>]*>)",
                QRegularExpression::CaseInsensitiveOption)))
    {
        errors.append("The response contains markdown or placeholder content.");
    }

    QRegularExpression fileRegex(
        R"((?:^|\n)FILE:\s*([^\r\n]+))");
    QRegularExpressionMatchIterator matches =
        fileRegex.globalMatch(response);
    QStringList fileNames;
    bool hasLanguageSource = false;
    const QStringList extensions = allowedExtensions(language);

    while (matches.hasNext())
    {
        const QString fileName = matches.next().captured(1).trimmed();
        const QFileInfo fileInfo(fileName);
        const QString extension = fileInfo.suffix().toLower();

        fileNames.append(fileName);

        if (fileName.contains('/')
            || fileName.contains('\\')
            || fileName.contains(':')
            || fileName == "."
            || fileName == "..")
        {
            errors.append("Unsafe generated file name: " + fileName);
        }

        if (!extensions.contains(extension))
        {
            errors.append(
                "Unexpected file extension for "
                + ProgrammingLanguage::languageTypeToText(language)
                + ": " + fileName);
        }

        if (extension != "txt"
            && extension != "h"
            && extension != "hpp")
        {
            hasLanguageSource = true;
        }
    }

    if (fileNames.isEmpty())
        errors.append("No FILE sections were generated.");
    else if (!hasLanguageSource)
        errors.append("No language source file was generated.");

    if (options.repository
        && !hasSecureBindingMarkers(response, language))
    {
        errors.append(
            "The data access code does not contain the required prepared-statement parameter binding.");
    }

    const QList<QRegularExpression> unsafeSqlPatterns = {
        QRegularExpression(
            R"((execute|executemany)\s*\(\s*f["'])",
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(
            R"((prepare|setQuery|QSqlQuery|execute)\s*\([^\r\n)]*(\+|sprintf|format\())",
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(
            R"(createStatement\s*\()",
            QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(
            R"((fmt::format|fmt\.Sprintf|format!)\s*\(\s*["'][^"']*(SELECT|INSERT|UPDATE|DELETE))",
            QRegularExpression::CaseInsensitiveOption)
    };

    for (const QRegularExpression& pattern : unsafeSqlPatterns)
    {
        if (pattern.match(response).hasMatch())
        {
            errors.append(
                "Potential SQL construction from string formatting or concatenation was detected.");
            break;
        }
    }

    for (const QString& tableName : tableNames)
    {
        for (const QString& layer : options.requestedLayers())
        {
            bool found = false;
            for (const QString& fileName : fileNames)
            {
                if (fileMatchesLayer(
                        QFileInfo(fileName).completeBaseName(),
                        tableName,
                        layer))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                errors.append(
                    "Missing " + layer + " file for table " + tableName + ".");
            }
        }
    }

    if (options.unitTests)
    {
        const bool hasTestFile = std::any_of(
            fileNames.cbegin(),
            fileNames.cend(),
            [](const QString& fileName)
            {
                return canonicalName(fileName).contains("test");
            });

        if (!hasTestFile)
            errors.append("Unit tests were requested but no test file was generated.");
    }

    errors.removeDuplicates();
    return errors;
}
