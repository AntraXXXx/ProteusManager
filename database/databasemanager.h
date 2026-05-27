#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>

class DatabaseManager
{
public:
    bool openDatabase(QString& connectionName, QString& databasePath);

    bool executeQuery(QString& sql);

    bool isLocalDatabase(bool isLocal);

    void setConnection(const bool connected);

    void setDatabasePath(const QString& path);

    bool isConnected();

private:
    QString m_databasePath;
    bool m_isConnected;
};


#endif
