#ifndef PARSEDCLASS_H
#define PARSEDCLASS_H

#include <QString>
#include <QList>
#include "parsedattribute.h"

struct ParsedClass
{
    QString name;
    QList<ParsedAttribute> attributes;
};

#endif