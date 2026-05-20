#include "textfilemanager.h"
#include <QFile>
#include <QTextStream>

QString TextFileManager::readFile(const QString& path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "";

    QTextStream in(&file);

    QString content = in.readAll();

    file.close();

    return content;
}

bool TextFileManager::writeFile(const QString& path,
                                const QString& content)
{
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);

    out << content;

    file.close();

    return true;
}