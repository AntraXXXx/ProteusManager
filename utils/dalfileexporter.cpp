#include "dalfileexporter.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

bool DalExportResult::success() const
{
    return filesSaved > 0;
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
