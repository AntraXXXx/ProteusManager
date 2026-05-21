#ifndef CLASSPARSER_H
#define CLASSPARSER_H

#include <QString>
#include <QList>
#include "parsedclass.h"

class ClassParser
{
public:
    QList<ParsedClass> parseCppClasses(const QString& content);

private:
    bool isKnownPrimitiveType(const QString& type) const;
};

#endif