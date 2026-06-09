#ifndef DALFILEEXPORTER_H
#define DALFILEEXPORTER_H

#include <QString>
#include <QStringList>

struct DalExportResult
{
    int filesSaved = 0;
    QString message;
    QStringList savedFilePaths;

    bool success() const;
};

class DalFileExporter
{
public:
    static DalExportResult exportFiles(
        const QString& response,
        const QString& outputPath);
};

#endif
