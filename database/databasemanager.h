#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>

class DatabaseManager
{
public:
    bool openDatabase(QString& connectionName, QString& databasePath);
    bool isLocalDatabase(bool isLocal);
    bool isConnected() const;
    bool executeQuery(const QString& executeSqlCommand);
    bool tableExists(const QString& tableName);
    bool columnExists(const QString& tableName, const QString& columnName);
    bool hasRows(const QString& tableName);
    bool isValidSql(const QString& sql);
    void setConnection(const bool connected);
    void setDatabasePath(const QString& path);
public:
    QString getSqlConnectionName() const;
    QStringList getColumnNames(const QString& tableName);
private:
    QString m_databasePath;
    bool m_isConnected;
    QString m_dataBaseConnectionName;
};


#endif
