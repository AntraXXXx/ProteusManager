#include "databasemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QDebug>

bool DatabaseManager::openDatabase(QString& connectionName, QString& databasePath)
{
    QSqlDatabase db =
        QSqlDatabase::addDatabase(
            "QSQLITE",
            connectionName
            );

    db.setDatabaseName(connectionName);

    if (!db.open())
    {
        qDebug() << "Database Error:"
                 << db.lastError().text();
    // isConnected = false;
        setConnection(false);
        return false;
    }

    qDebug() << "Database connected";
  //  isConnected = true;
    setConnection(true);
    return true;
}

bool DatabaseManager::executeQuery(QString& sql)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_databasePath);

    if (!db.isOpen())
    {
        qDebug() << "Database not open";
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec(sql))
    {
        qDebug() << "SQL Error:"
                 << query.lastError().text();

        qDebug() << "SQL:"
                 << sql;

        return false;
    }

    qDebug() << "SQL executed successfully";

    return true;
}

void DatabaseManager::setDatabasePath(const QString& path)
{
    m_databasePath = path;
}

void DatabaseManager::setConnection(const bool isConnecting)
{
    m_isConnected = isConnecting;
    isConnected();
}

bool DatabaseManager::isConnected()
{
    return m_isConnected;
}




