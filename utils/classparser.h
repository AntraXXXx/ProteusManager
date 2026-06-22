#ifndef CLASSPARSER_H
#define CLASSPARSER_H

#include <QString>
#include <QList>
#include "parsedclass.h"
#include "programminglanguage.h"

class ClassParser
{
public:
    QList<ParsedClass> parseClasses(
        const QString& content,
        ProgrammingLanguage::ProgrammingLanguageType language);

private:
    QList<ParsedClass> parseCppClasses(const QString& content);
    QList<ParsedClass> parseCClasses(const QString& content);
    QList<ParsedClass> parseCsharpClasses(const QString& content);
    QList<ParsedClass> parsePythonClasses(const QString& content);
    QList<ParsedClass> parseGoClasses(const QString& content);
    QList<ParsedClass> parseRustClasses(const QString& content);
    QList<ParsedClass> parseFsharpClasses(const QString& content);
    QList<ParsedClass> parsePowershellClasses(const QString& content);
    QList<ParsedClass> parseJavaClasses(const QString& content);
    bool isRelationType(
        const QString& type,
        ProgrammingLanguage::ProgrammingLanguageType language) const;
};

#endif
