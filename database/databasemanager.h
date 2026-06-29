#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QStringList>
#include <QVariantList>

class DatabaseManager
{
public:
    bool openDatabase(const QString& connectionName, const QString& databasePath);
    bool openRemoteDatabase(
        const QString& connectionName,
        const QString& driver,
        const QString& hostName,
        int port,
        const QString& databaseName,
        const QString& userName,
        const QString& password);
    bool isLocalDatabase(bool isLocal);
    bool isConnected() const;
    bool executeQuery(const QString& executeSqlCommand);
    bool tableExists(const QString& tableName);
    bool columnExists(const QString& tableName, const QString& columnName);
    bool hasRows(const QString& tableName);
    bool isValidSql(const QString& sql);
    bool isValidMigrationSql(const QString& sql) const;
    bool validateMigrationPreview(const QString& migrationSql);
    bool executeMigration(const QString& migrationSql);
    void setConnection(const bool connected);
    void setDatabasePath(const QString& path);
    QString getSqlConnectionName() const;
    QString buildSchemaDescription(
        const QStringList& tableNames = {});
    QString buildNormalizationAnalysis(
        const QStringList& tableNames = {},
        int sampleLimit = 5);
    bool hasNormalizationEvidence(
        const QStringList& tableNames = {});
    QStringList getColumnNames(const QString& tableName);
    QStringList getTableNames();
    QVariantList buildSchemaDiagram();
    QVariantList buildSchemaDiagramForTables(
        const QStringList& tableNames);
    QVariantList buildSchemaDiagramWithMigration(
        const QString& migrationSql);
    QStringList migrationTargetTableNames(
        const QString& migrationSql) const;
    QString databaseDriver() const;
    QString lastError() const;
private:
    QString m_databasePath;
    QString m_lastError;
    bool m_isConnected = false;
    QString m_dataBaseConnectionName;
    bool m_isValidSql = false;
};


#endif
