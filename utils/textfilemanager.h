#ifndef TEXTFILEMANAGER_H
#define TEXTFILEMANAGER_H

#pragma once

#include <QString>
#include <QDirIterator>

class TextFileManager
{
public:
    QString readFile(const QString& path);

    bool writeFile(const QString& path,
                   const QString& content);
};



#endif
