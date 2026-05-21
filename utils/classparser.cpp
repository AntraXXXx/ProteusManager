#include "classparser.h"
#include <QRegularExpression>

QList<ParsedClass> ClassParser::parseCppClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression classRegex(
        R"(class\s+(\w+)\s*\{([\s\S]*?)\};)"
        );

    auto classMatches = classRegex.globalMatch(content);

    while (classMatches.hasNext())
    {
        auto match = classMatches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attributeRegex(
            R"((QString|std::string|int|double|float|bool|QDate|QDateTime|\w+)\s+(\w+)\s*;)"
            );

        auto attributeMatches = attributeRegex.globalMatch(body); //regex for datatypes

        while (attributeMatches.hasNext())
        {
            auto fieldMatch = attributeMatches.next();

            ParsedAttribute attribute;
            attribute.type = fieldMatch.captured(1);
            attribute.name = fieldMatch.captured(2);
            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

bool ClassParser::isKnownPrimitiveType(const QString& type) const
{
    return type == "int"
           || type == "double"
           || type == "float"
           || type == "bool"
           || type == "QString"
           || type == "std::string"
           || type == "QDate"
           || type == "QDateTime";
}