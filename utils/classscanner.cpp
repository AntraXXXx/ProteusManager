#include "classscanner.h"

#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QDebug>

QList<ScannedClassFile> ClassScanner::scanAndReadClassFiles(const QString& folderPath)
{
    QList<ScannedClassFile> result;

    QDirIterator iterator(
        folderPath,
        QStringList() << "*.h",
        QDir::Files,
        QDirIterator::Subdirectories
        );

    while (iterator.hasNext())
    {
        QString filePath = iterator.next();
        QString content = readFileContent(filePath);

        if (!content.isEmpty())
        {
            ScannedClassFile file;
            file.filePath = filePath;
            file.content = content;

            result.append(file);

            qDebug() << "read:" << filePath;
        }
    }

    return result;
}

QString ClassScanner::readFileContent(const QString& filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Error: missing file" << filePath;
        return "";
    }

    QTextStream in(&file);
    return in.readAll();
}