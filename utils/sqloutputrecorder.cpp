#include "sqloutputrecorder.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

QString SqlOutputRecorder::defaultOutputDirectory()
{
    const QString appDataPath =
        QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);

    if (appDataPath.isEmpty())
        return QDir::temp().filePath("ProteusManager/generated-sql");

    return QDir(appDataPath).filePath("generated-sql");
}

QString SqlOutputRecorder::sanitizedModelName(
    const QString& modelName)
{
    QString sanitized =
        modelName.trimmed();

    if (sanitized.isEmpty())
        sanitized = "unknown-agent";

    sanitized.replace(
        QRegularExpression("[^A-Za-z0-9._-]+"),
        "_");

    return sanitized;
}

SqlOutputRecord SqlOutputRecorder::recordGeneratedSql(
    const QString& modelName,
    const QString& sql,
    const QString& outputDirectory)
{
    SqlOutputRecord record;

    if (sql.trimmed().isEmpty())
    {
        record.message = "No SQL output available.";
        return record;
    }

    const QString targetDirectory =
        outputDirectory.isEmpty()
            ? defaultOutputDirectory()
            : outputDirectory;

    QDir dir(targetDirectory);
    if (!dir.exists() && !dir.mkpath("."))
    {
        record.message = "SQL output directory could not be created.";
        return record;
    }

    const QString timestamp =
        QDateTime::currentDateTimeUtc()
            .toString("yyyyMMdd_HHmmss_zzz");

    const QString fileName =
        sanitizedModelName(modelName)
        + "_"
        + timestamp
        + ".sql";

    QFile file(dir.filePath(fileName));

    if (!file.open(
            QIODevice::WriteOnly |
            QIODevice::Text))
    {
        record.message = "SQL output file could not be written.";
        return record;
    }

    QTextStream out(&file);
    out << sql.trimmed() << "\n";
    file.close();

    record.saved = true;
    record.filePath = file.fileName();
    record.message = "SQL output saved.";

    return record;
}
