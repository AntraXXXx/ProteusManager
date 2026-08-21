#include "sqlsafetypolicy.h"

#include <QRegularExpression>

QStringList SqlSafetyPolicy::splitStatements(
    const QString& sql)
{
    return sql.split(';', Qt::SkipEmptyParts);
}

QString SqlSafetyPolicy::stripLeadingComments(
    QString statement)
{
    statement = statement.trimmed();

    while (statement.startsWith("--"))
    {
        const int lineEnd = statement.indexOf('\n');
        if (lineEnd == -1)
            return {};

        statement = statement.mid(lineEnd + 1).trimmed();
    }

    return statement;
}

bool SqlSafetyPolicy::isValidSchemaSql(
    const QString& sql)
{
    const QString trimmedSql = sql.trimmed();
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
    for (const QString& statement : splitStatements(trimmedSql))
    {
        const QString compact = statement.trimmed().simplified();
        if (compact.isEmpty())
            continue;

        if (destructiveStatement.match(compact).hasMatch())
            return false;

        const bool allowed =
            createTableStatement.match(compact).hasMatch()
            || createIndexStatement.match(compact).hasMatch()
            || alterAddColumnStatement.match(compact).hasMatch();
        if (!allowed)
            return false;

        hasValidStatement = true;
    }

    return hasValidStatement;
}

bool SqlSafetyPolicy::isValidMigrationSql(
    const QString& sql)
{
    if (sql.trimmed().isEmpty())
        return false;

    const QRegularExpression forbiddenStatement(
        "\\b(DROP|DELETE|TRUNCATE|UPDATE|REPLACE|ATTACH|DETACH|VACUUM|BEGIN|COMMIT|ROLLBACK)\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression alterDropStatement(
        "^ALTER\\s+TABLE\\b[\\s\\S]*\\bDROP\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression createTableStatement(
        "^CREATE\\s+TABLE\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression createIndexStatement(
        "^CREATE\\s+(UNIQUE\\s+)?INDEX\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression insertSelectStatement(
        "^INSERT(?:\\s+OR\\s+IGNORE|\\s+IGNORE)?\\s+INTO\\b[\\s\\S]*\\bSELECT\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression commonTableExpressionInsertSelectStatement(
        "^WITH(?:\\s+RECURSIVE)?\\b[\\s\\S]*\\bINSERT(?:\\s+OR\\s+IGNORE|\\s+IGNORE)?\\s+INTO\\b[\\s\\S]*\\bSELECT\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression alterSafeStatement(
        "^ALTER\\s+TABLE\\b[\\s\\S]*\\b(ADD|RENAME)\\b",
        QRegularExpression::CaseInsensitiveOption);

    bool hasStatement = false;
    for (QString statement : splitStatements(sql))
    {
        statement = stripLeadingComments(statement);
        const QString compact = statement.simplified();
        if (compact.isEmpty())
            continue;

        if (forbiddenStatement.match(compact).hasMatch()
            || alterDropStatement.match(compact).hasMatch())
        {
            return false;
        }

        const bool allowed =
            createTableStatement.match(compact).hasMatch()
            || createIndexStatement.match(compact).hasMatch()
            || insertSelectStatement.match(compact).hasMatch()
            || commonTableExpressionInsertSelectStatement.match(compact).hasMatch()
            || alterSafeStatement.match(compact).hasMatch();
        if (!allowed)
            return false;

        hasStatement = true;
    }

    return hasStatement;
}

QString SqlSafetyPolicy::incompatibleFeature(
    const QString& driverName,
    const QString& sql)
{
    QString pattern;

    if (driverName == "QSQLITE")
    {
        pattern =
            "\\b(CHARINDEX|STRING_SPLIT|SPLIT_PART|UNNEST|GENERATE_SERIES|CONCAT)\\s*\\("
            "|\\bTOP\\s+\\d+\\b|\\bAUTO_INCREMENT\\b|\\bIDENTITY\\s*\\(";
    }
    else if (driverName.startsWith("QPSQL"))
    {
        pattern =
            "\\b(INSTR|IFNULL|CHARINDEX|STRING_SPLIT)\\s*\\("
            "|`|\\bAUTO_INCREMENT\\b|\\bIDENTITY\\s*\\(";
    }
    else if (driverName.startsWith("QMYSQL"))
    {
        pattern =
            "\\b(CHARINDEX|STRING_SPLIT|SPLIT_PART|UNNEST)\\s*\\("
            "|\\bTOP\\s+\\d+\\b|\\bIDENTITY\\s*\\(";
    }
    else if (driverName.startsWith("QODBC"))
    {
        pattern =
            "\\b(INSTR|SUBSTR|STRPOS|SPLIT_PART|UNNEST)\\s*\\("
            "|\\bLIMIT\\s+\\d+\\b|`|\\bAUTO_INCREMENT\\b|\\|\\|";
    }

    if (pattern.isEmpty())
        return {};

    const QRegularExpression incompatible(
        pattern,
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = incompatible.match(sql);
    return match.hasMatch() ? match.captured(0) : QString();
}
