#include "dalfileexporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

namespace
{
QString canonicalName(QString value)
{
    value = value.toLower();
    value.remove(QRegularExpression("[^a-z0-9]"));
    return value;
}

QString pascalCaseTableName(QString tableName)
{
    const int schemaSeparator = tableName.lastIndexOf('.');
    if (schemaSeparator >= 0)
        tableName = tableName.mid(schemaSeparator + 1);

    const QStringList parts =
        tableName.split(
            QRegularExpression("[^A-Za-z0-9]+"),
            Qt::SkipEmptyParts);

    QString result;
    for (QString part : parts)
    {
        if (part == part.toUpper())
            part = part.toLower();

        if (!part.isEmpty())
            part[0] = part.at(0).toUpper();

        result += part;
    }

    return result;
}

QString tableForGeneratedBaseName(
    const QString& generatedBaseName,
    const QStringList& tableNames)
{
    const QString generatedKey =
        canonicalName(generatedBaseName);

    for (const QString& tableName : tableNames)
    {
        const QString tableKey =
            canonicalName(tableName);

        const QStringList acceptedNames = {
            tableKey,
            "sql" + tableKey,
            tableKey + "repository",
            tableKey + "repositoryinterface",
            tableKey + "dao",
            tableKey + "dal",
            tableKey + "access",
            tableKey + "dataaccess"
        };

        if (acceptedNames.contains(generatedKey))
            return tableName;
    }

    return {};
}

QString replaceClassName(
    QString content,
    const QString& oldClassName,
    const QString& newClassName,
    const QString& tableName)
{
    if (oldClassName.isEmpty()
        || oldClassName == newClassName)
    {
        return content;
    }

    const QString escapedOldName =
        QRegularExpression::escape(oldClassName);

    if (canonicalName(oldClassName)
        != canonicalName(tableName))
    {
        content.replace(
            QRegularExpression("\\b" + escapedOldName + "\\b"),
            newClassName);
        return content;
    }

    content.replace(
        QRegularExpression("\\b" + escapedOldName + "::"),
        newClassName + "::");
    content.replace(
        QRegularExpression("\\b" + escapedOldName + "(?=\\s*\\()"),
        newClassName);
    content.replace(
        QRegularExpression("(\\bclass\\s+)" + escapedOldName + "\\b"),
        "\\1" + newClassName);
    content.replace(
        QRegularExpression("(\\bstruct\\s+)" + escapedOldName + "\\b"),
        "\\1" + newClassName);

    return content;
}

QString findGeneratedClassName(
    const QString& content,
    const QString& tableName)
{
    QRegularExpression classRegex(
        "\\b(?:class|struct)\\s+([A-Za-z_][A-Za-z0-9_]*)");
    QRegularExpressionMatchIterator classMatches =
        classRegex.globalMatch(content);

    while (classMatches.hasNext())
    {
        const QString className =
            classMatches.next().captured(1);

        if (!tableForGeneratedBaseName(
                className,
                {tableName}).isEmpty())
        {
            return className;
        }
    }

    QRegularExpression qualifierRegex(
        "\\b([A-Za-z_][A-Za-z0-9_]*)::");
    QRegularExpressionMatchIterator qualifierMatches =
        qualifierRegex.globalMatch(content);

    while (qualifierMatches.hasNext())
    {
        const QString className =
            qualifierMatches.next().captured(1);

        if (!tableForGeneratedBaseName(
                className,
                {tableName}).isEmpty())
        {
            return className;
        }
    }

    return {};
}

QString replaceGeneratedFileReferences(
    QString content,
    const QString& oldBaseName,
    const QString& newBaseName)
{
    const QStringList extensions = {
        "h", "hpp", "cpp", "cc", "cxx", "cs"
    };

    for (const QString& extension : extensions)
    {
        content.replace(
            QRegularExpression(
                QRegularExpression::escape(
                    oldBaseName + "." + extension),
                QRegularExpression::CaseInsensitiveOption),
            newBaseName + "." + extension);
    }

    return content;
}

QString replaceHeaderGuard(
    QString content,
    const QString& newBaseName,
    const QString& suffix)
{
    QRegularExpression guardRegex(
        "#ifndef\\s+([A-Za-z0-9_]+)");
    QRegularExpressionMatch guardMatch =
        guardRegex.match(content);

    if (!guardMatch.hasMatch())
        return content;

    const QString oldGuard =
        guardMatch.captured(1);
    QString newGuard =
        newBaseName.toUpper()
        + "_"
        + suffix.toUpper();
    newGuard.replace(
        QRegularExpression("[^A-Z0-9_]"),
        "_");

    content.replace(
        QRegularExpression("\\b" + QRegularExpression::escape(oldGuard) + "\\b"),
        newGuard);

    return content;
}
}

bool DalExportResult::success() const
{
    return filesSaved > 0;
}

QString DalFileExporter::applySqlNamingConvention(
    const QString& response,
    const QStringList& tableNames)
{
    if (response.isEmpty()
        || tableNames.isEmpty())
    {
        return response;
    }

    const QStringList sections =
        response.split(
            "FILE:",
            Qt::SkipEmptyParts);

    QStringList normalizedSections;

    for (const QString& section : sections)
    {
        const QString trimmed =
            section.trimmed();
        const int lineEnd =
            trimmed.indexOf('\n');

        if (lineEnd == -1)
            continue;

        const QString rawFileName =
            trimmed.left(lineEnd).trimmed();
        const QFileInfo fileInfo(rawFileName);
        const QString oldBaseName =
            fileInfo.completeBaseName();
        const QString suffix =
            fileInfo.suffix();
        const QString tableName =
            tableForGeneratedBaseName(
                oldBaseName,
                tableNames);

        QString content =
            trimmed.mid(lineEnd + 1);
        QString normalizedFileName =
            rawFileName;

        if (!tableName.isEmpty()
            && !suffix.isEmpty())
        {
            const QString newBaseName =
                "Sql" + pascalCaseTableName(tableName);
            const QString oldClassName =
                findGeneratedClassName(
                    content,
                    tableName);

            normalizedFileName =
                newBaseName + "." + suffix.toLower();
            content = replaceClassName(
                content,
                oldClassName,
                newBaseName,
                tableName);
            content = replaceGeneratedFileReferences(
                content,
                oldBaseName,
                newBaseName);

            if (suffix.compare("h", Qt::CaseInsensitive) == 0
                || suffix.compare("hpp", Qt::CaseInsensitive) == 0)
            {
                content = replaceHeaderGuard(
                    content,
                    newBaseName,
                    suffix);
            }
        }

        normalizedSections.append(
            "FILE: "
            + normalizedFileName
            + "\n"
            + content.trimmed());
    }

    return normalizedSections.isEmpty()
               ? response
               : normalizedSections.join("\n\n");
}

DalExportResult DalFileExporter::exportFiles(
    const QString& response,
    const QString& outputPath)
{
    DalExportResult result;

    if (response.isEmpty())
    {
        result.message = "No DAL code available.";
        return result;
    }

    if (outputPath.isEmpty())
    {
        result.message = "No output path selected.";
        return result;
    }

    QDir outputDir(outputPath);
    if (!outputDir.exists())
    {
        result.message = "Selected output path does not exist.";
        return result;
    }

    const QStringList sections =
        response.split(
            "FILE:",
            Qt::SkipEmptyParts
            );

    for (const QString& section : sections)
    {
        const QString trimmed =
            section.trimmed();

        const int lineEnd =
            trimmed.indexOf('\n');

        if (lineEnd == -1)
            continue;

        const QString rawFileName =
            trimmed.left(lineEnd).trimmed();

        if (rawFileName.isEmpty()
            || rawFileName.contains('/')
            || rawFileName.contains('\\')
            || rawFileName.contains(':'))
        {
            continue;
        }

        const QString fileContent =
            trimmed.mid(lineEnd + 1);

        QFile file(outputDir.filePath(rawFileName));

        if (!file.open(
                QIODevice::WriteOnly |
                QIODevice::Text))
        {
            continue;
        }

        QTextStream out(&file);
        out << fileContent;
        file.close();

        ++result.filesSaved;
        result.savedFilePaths.append(file.fileName());
    }

    if (!result.success())
    {
        result.message =
            "No valid DAL files were found in the AI response.";
        return result;
    }

    result.message =
        QString("DAL files saved: %1").arg(result.filesSaved);

    return result;
}
