#ifndef CLASSSCANNER_H
#define CLASSSCANNER_H

#include <QString>
#include <QStringList>
#include <QList>
#include "programminglanguage.h"

struct ScannedClassFile
{
    QString filePath;
    QString content;
};

class ClassScanner
{
public:
    QList<ScannedClassFile> scanAndReadClassFiles(
        const QString& folderPath,
        ProgrammingLanguage::ProgrammingLanguageType language);

    QStringList fileFiltersForLanguage(
        ProgrammingLanguage::ProgrammingLanguageType language) const;

private:
    QString readFileContent(const QString& filePath);
};

#endif