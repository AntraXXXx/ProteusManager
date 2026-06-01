#include "classscanner.h"

#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QDebug>

QList<ScannedClassFile> ClassScanner::scanAndReadClassFiles(
    const QString& folderPath,
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    QList<ScannedClassFile> result;

    QDirIterator iterator(
        folderPath,
        fileFiltersForLanguage(language),
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

QStringList ClassScanner::fileFiltersForLanguage(
    ProgrammingLanguage::ProgrammingLanguageType language) const
{
    switch (language)
    {
    case ProgrammingLanguage::ProgrammingLanguageType::Cplusplus:
        return QStringList() << "*.h" << "*.hpp";//  later *.cpp

    case ProgrammingLanguage::ProgrammingLanguageType::Csharp:
        return QStringList() << "*.cs";

    case ProgrammingLanguage::ProgrammingLanguageType::C:
        return QStringList() << "*.c" << "*.h";

    case ProgrammingLanguage::ProgrammingLanguageType::Java:
        return QStringList() << "*.java";

    case ProgrammingLanguage::ProgrammingLanguageType::Python:
        return QStringList() << "*.py";

    case ProgrammingLanguage::ProgrammingLanguageType::Rust:
        return QStringList() << "*.rs";

    case ProgrammingLanguage::ProgrammingLanguageType::Powershell:
        return QStringList() << "*.ps1";

    case ProgrammingLanguage::ProgrammingLanguageType::Go:
        return QStringList() << "*.go";

    case ProgrammingLanguage::ProgrammingLanguageType::Fsharp:
        return QStringList() << "*.fs" << "*.fsx";
    }

    return QStringList() << "*.h";
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