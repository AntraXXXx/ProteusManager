#ifndef SQLSAFETYPOLICY_H
#define SQLSAFETYPOLICY_H

#include <QString>
#include <QStringList>

class SqlSafetyPolicy
{
public:
    static QStringList splitStatements(const QString& sql);
    static QString stripLeadingComments(QString statement);
    static bool isValidSchemaSql(const QString& sql);
    static bool isValidMigrationSql(const QString& sql);
    static QString incompatibleFeature(
        const QString& driverName,
        const QString& sql);
};

#endif
