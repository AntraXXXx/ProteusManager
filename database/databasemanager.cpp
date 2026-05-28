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



