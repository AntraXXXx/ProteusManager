#ifndef PARSEDATTRIBUTE_H
#define PARSEDATTRIBUTE_H

#include <QString>

struct ParsedAttribute
{
    QString type;
    QString name;
    bool isRelation = false;
};

#endif
