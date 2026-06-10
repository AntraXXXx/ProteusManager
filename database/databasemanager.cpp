#include "databasemanager.h"

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>

#include <QDebug>
#include <QRegularExpression>

namespace
{
QString quoteSqliteIdentifier(const QString& identifier)
{
    QString quoted = identifier;
    quoted.replace("\"", "\"\"");
    return "\"" + quoted + "\"";
}

QString escapeTableIdentifier(const QSqlDatabase& db, const QString& tableName)
{
    const QString escaped =
        db.driver()
            ? db.driver()->escapeIdentifier(tableName, QSqlDriver::TableName)
            : QString();

    if (!escaped.isEmpty())
        return escaped;

    return quoteSqliteIdentifier(tableName);
}
}

bool DatabaseManager::openDatabase(const QString& connectionName,
                                   const QString& databasePath)
{
    if (connectionName.isEmpty() || databasePath.isEmpty())
    {
        m_lastError = "No local database path selected.";
        setConnection(false);
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::contains(connectionName)
            ? QSqlDatabase::database(connectionName)
            : QSqlDatabase::addDatabase(
                  "QSQLITE",
                  connectionName
                  );

    m_dataBaseConnectionName = connectionName;

    if (db.isOpen())
        db.close();

    db.setDatabaseName(databasePath);

    if (!db.open())
    {
        m_lastError = db.lastError().text();
        qDebug() << "Database Error:"
                 << m_lastError;

        setConnection(false);
        return false;
    }

    m_databasePath = databasePath;
    m_lastError.clear();
    qDebug() << "Database connected";

    setConnection(true);
    return true;
}

bool DatabaseManager::openRemoteDatabase(
    const QString& connectionName,
    const QString& driver,
    const QString& hostName,
    int port,
    const QString& databaseName,
    const QString& userName,
    const QString& password)
{
    if (connectionName.isEmpty()
        || driver.isEmpty()
        || hostName.isEmpty()
        || databaseName.isEmpty())
    {
        m_lastError =
            "Database type, database name and host name are required.";
        setConnection(false);
        return false;
    }

    if (!QSqlDatabase::drivers().contains(driver))
    {
        m_lastError =
            "Qt SQL driver is not installed: " + driver;
        qDebug() << m_lastError;
        setConnection(false);
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::contains(connectionName)
            ? QSqlDatabase::database(connectionName)
            : QSqlDatabase::addDatabase(
                  driver,
                  connectionName
                  );

    m_dataBaseConnectionName = connectionName;

    if (db.isOpen())
        db.close();

    db.setHostName(hostName);
    db.setDatabaseName(databaseName);
    db.setUserName(userName);
    db.setPassword(password);

    if (port > 0)
        db.setPort(port);

    if (!db.open())
    {
        m_lastError = db.lastError().text();
        qDebug() << "Remote database error:"
                 << m_lastError;

        setConnection(false);
        return false;
    }

    m_databasePath = databaseName;
    m_lastError.clear();
    qDebug() << "Remote database connected";

    setConnection(true);
    return true;
}

QString DatabaseManager::getSqlConnectionName() const
{
    return m_dataBaseConnectionName;
}

bool DatabaseManager::executeQuery(const QString& executeSqlCommand)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
    {
        m_lastError = "Database is not open.";
        qDebug() << "Database is not open";
        qDebug() << "Connection name:" << m_dataBaseConnectionName;
        return false;
    }

    QStringList queries =
        executeSqlCommand.split(";", Qt::SkipEmptyParts);

    for (QString queryString : queries)
    {
        queryString = queryString.trimmed();

        if (queryString.isEmpty())
            continue;

        QSqlQuery query(db);

        if (!query.exec(queryString))
        {
            m_lastError = query.lastError().text();
            qDebug() << "SQL Error:" << query.lastError().text();
            qDebug() << "Failed Query:" << queryString;
            m_isValidSql = false;
            return false;
        }

        qDebug() << "Executed:" << queryString;
    }
    m_isValidSql = true;
    return true;
}

void DatabaseManager::setDatabasePath(const QString& path)
{
    m_databasePath = path;
}

void DatabaseManager::setConnection(const bool isConnecting)
{
    m_isConnected = isConnecting;
}

bool DatabaseManager::isConnected() const
{
    return m_isConnected;
}

bool DatabaseManager::isLocalDatabase(bool isLocal)
{
    return isLocal;
}

bool DatabaseManager::tableExists(const QString& tableName)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
    {
        m_lastError = "Database is not open.";
        qDebug() << "Database is not open";
        return false;
    }

    const QStringList tables =
        db.tables(QSql::Tables);

    for (const QString& table : tables)
    {
        if (table.compare(tableName, Qt::CaseInsensitive) == 0)
            return true;
    }

    return false;
}

QStringList DatabaseManager::getTableNames()
{
    QSqlDatabase db = QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
        return {};

    QStringList tables =
        db.tables(QSql::Tables);

    tables.removeAll("sqlite_sequence");

    return tables;
}

QString DatabaseManager::buildSchemaDescription()
{
    QString result =
        "Current "
        + databaseDriver()
        + " database schema:\n\n";

    for (const QString& table : getTableNames())
    {
        result += "Table: " + table + "\n";

        QSqlDatabase db =
            QSqlDatabase::database(m_dataBaseConnectionName);

        if (db.driverName() == "QSQLITE")
        {
            QSqlQuery query(
                QString("PRAGMA table_info(%1)")
                    .arg(quoteSqliteIdentifier(table)),
                db);

            while (query.next())
            {
                result += "- "
                          + query.value("name").toString()
                          + " "
                          + query.value("type").toString()
                          + "\n";
            }
        }
        else
        {
            const QSqlRecord record =
                db.record(table);

            for (int i = 0; i < record.count(); ++i)
            {
                const QSqlField field =
                    record.field(i);

                result += "- "
                          + field.name()
                          + " "
                          + QString::fromUtf8(field.metaType().name())
                          + "\n";
            }
        }

        result += "\n";
    }

    return result;
}

QString DatabaseManager::databaseDriver() const
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isValid())
        return "database";

    return db.driverName();
}

bool DatabaseManager::columnExists(
    const QString& tableName,
    const QString& columnName)
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isOpen())
        return false;

    const QSqlRecord record =
        db.record(tableName);

    for (int i = 0; i < record.count(); ++i)
    {
        if (record.fieldName(i).compare(
                columnName,
                Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }

    return false;
}

bool DatabaseManager::hasRows(
    const QString& tableName)
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    QSqlQuery query(db);

    query.exec(
        QString(
            "SELECT COUNT(*) FROM %1")
            .arg(escapeTableIdentifier(db, tableName)));

    if (query.next())
    {
        return query.value(0).toInt() > 0;
    }

    return false;
}

QStringList DatabaseManager::getColumnNames(
    const QString& tableName)
{
    QStringList columns;

    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isOpen())
        return columns;

    const QSqlRecord record =
        db.record(tableName);

    for (int i = 0; i < record.count(); ++i)
    {
        columns.append(
            record.fieldName(i)
            );
    }

    return columns;
}

bool DatabaseManager::isValidSql(
    const QString& sql)
{
    const QString trimmedSql =
        sql.trimmed();

    if (trimmedSql.isEmpty())
        return false;

    const QRegularExpression destructiveStatement(
        "\\b(DROP|DELETE|TRUNCATE|UPDATE|INSERT|REPLACE|ATTACH|DETACH|VACUUM)\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression createTableStatement(
        "^CREATE\\s+TABLE\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression createIndexStatement(
        "^CREATE\\s+(UNIQUE\\s+)?INDEX\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression alterAddColumnStatement(
        "^ALTER\\s+TABLE\\b[\\s\\S]*\\bADD\\s+COLUMN\\b",
        QRegularExpression::CaseInsensitiveOption);

    bool hasValidStatement = false;

    const QStringList statements =
        trimmedSql.split(
            ";",
            Qt::SkipEmptyParts);

    for (const QString& statement : statements)
    {
        const QString compactStatement =
            statement.trimmed().simplified();

        if (compactStatement.isEmpty())
            continue;

        if (destructiveStatement.match(compactStatement).hasMatch())
            return false;

        const bool isAllowedSchemaStatement =
            createTableStatement.match(compactStatement).hasMatch()
            || createIndexStatement.match(compactStatement).hasMatch()
            || alterAddColumnStatement.match(compactStatement).hasMatch();

        if (!isAllowedSchemaStatement)
            return false;

        hasValidStatement = true;
    }

    return hasValidStatement;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

