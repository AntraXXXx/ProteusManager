#ifndef CLASSSCANNER_H
#define CLASSSCANNER_H

#include <QString>
#include <QStringList>
#include <QList>

struct ScannedClassFile
{
    QString filePath;
    QString content;
};

class ClassScanner
{
public:
    QList<ScannedClassFile> scanAndReadClassFiles(const QString& folderPath);

private:
    QString readFileContent(const QString& filePath);
};

#endif