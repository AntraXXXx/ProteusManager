#include "databasemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

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
}

bool DatabaseManager::openDatabase(const QString& connectionName,
                                   const QString& databasePath)
{
    if (connectionName.isEmpty() || databasePath.isEmpty())
    {
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
        qDebug() << "Database Error:"
                 << db.lastError().text();

        setConnection(false);
        return false;
    }
    qDebug() << "Database connected";

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

bool DatabaseManager::tableExists(const QString& tableName)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
    {
        qDebug() << "Database is not open";
        return false;
    }

    QSqlQuery query(db);

    query.prepare(
        "SELECT name FROM sqlite_master "
        "WHERE type='table' AND name=:tableName"
        );

    query.bindValue(":tableName", tableName);

    if (!query.exec())
    {
        qDebug() << "Table check error:"
                 << query.lastError().text();
        m_isValidSql = false;
        return false;
    }
    return query.next();
}

QStringList DatabaseManager::getTableNames()
{
    QStringList tables;

    QSqlDatabase db = QSqlDatabase::database(m_dataBaseConnectionName);
    QSqlQuery query(db);

    query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");

    while (query.next())
        tables.append(query.value(0).toString());

    return tables;
}

QString DatabaseManager::buildSchemaDescription()
{
    QString result = "Current SQLite database schema:\n\n";

    for (const QString& table : getTableNames())
    {
        result += "Table: " + table + "\n";

        QSqlQuery query(
            QString("PRAGMA table_info(%1)").arg(quoteSqliteIdentifier(table)),
            QSqlDatabase::database(m_dataBaseConnectionName)
            );

        while (query.next())
        {
            result += "- "
                      + query.value("name").toString()
                      + " "
                      + query.value("type").toString()
                      + "\n";
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

    return db.driverName();
}

bool DatabaseManager::columnExists(
    const QString& tableName,
    const QString& columnName)
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    QSqlQuery query(
        QString("PRAGMA table_info(%1)")
            .arg(quoteSqliteIdentifier(tableName)),
        db);

    while (query.next())
    {
        QString column =
            query.value("name").toString();

        if (column.compare(
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
            .arg(quoteSqliteIdentifier(tableName)));

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

    QSqlQuery query(
        QString("PRAGMA table_info(%1)")
            .arg(quoteSqliteIdentifier(tableName)),
        db);

    while (query.next())
    {
        columns.append(
            query.value("name").toString()
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

