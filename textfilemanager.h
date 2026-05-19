#ifndef TEXTFILEMANAGER_H
#define TEXTFILEMANAGER_H

#pragma once

#include <QString>

class TextFileManager
{
public:
    QString readFile(const QString& path);

    bool writeFile(const QString& path,
                   const QString& content);

    bool appendToFile(const QString& path,
                      const QString& content);
};



#endif // TEXTFILEMANAGER_H
