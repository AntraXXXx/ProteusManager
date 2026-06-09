#ifndef SQLOUTPUTRECORDER_H
#define SQLOUTPUTRECORDER_H

#include <QString>

struct SqlOutputRecord
{
    bool saved = false;
    QString filePath;
    QString message;
};

class SqlOutputRecorder
{
public:
    static SqlOutputRecord recordGeneratedSql(
        const QString& modelName,
        const QString& sql,
        const QString& outputDirectory = QString());

    static QString defaultOutputDirectory();
    static QString sanitizedModelName(const QString& modelName);
};

#endif
