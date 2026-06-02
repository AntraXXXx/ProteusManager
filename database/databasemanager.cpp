#include "databasemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QDebug>

bool DatabaseManager::openDatabase(QString& connectionName,
                                   QString& databasePath)
{
    QSqlDatabase db =
        QSqlDatabase::addDatabase(
            "QSQLITE",
            connectionName
            );

    m_dataBaseConnectionName = connectionName;

    db.setDatabaseName(databasePath);

    if (!db.open() || connectionName.isEmpty() || databasePath.isEmpty())
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
            return false;
        }

        qDebug() << "Executed:" << queryString;
    }

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

        return false;
    }

    return query.next();
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
            .arg(tableName),
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
            .arg(tableName));

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
            .arg(tableName),
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
    QString upperSql = sql.toUpper();

    return upperSql.contains("CREATE TABLE");
}

