#include "classparser.h"
#include <QRegularExpression>

QList<ParsedClass> ClassParser::parseClasses(
    const QString& content,
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    switch (language)
    {
    case ProgrammingLanguage::ProgrammingLanguageType::Cplusplus:
        return parseCppClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::C:
        return parseCClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Csharp:
        return parseCsharpClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Python:
        return parsePythonClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Go:
         return parseGoClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Rust:
         return parseRustClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Fsharp:
         return parseFsharpClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Powershell:
         return parsePowershellClasses(content);
    case ProgrammingLanguage::ProgrammingLanguageType::Java:
         return parseJavaClasses(content);
    default:
        return {};
    }
}

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

        QRegularExpression memberRegex(
            R"((?:^|\n)\s*(?:const\s+)?([\w:<>]+)\s+(m_\w+)\s*(?:\{[^;]*\})?\s*;)"
            );

        auto memberMatches = memberRegex.globalMatch(body);

        while (memberMatches.hasNext())
        {
            auto fieldMatch = memberMatches.next();

            ParsedAttribute attribute;
            attribute.type = fieldMatch.captured(1);
            attribute.name = fieldMatch.captured(2);

            if (attribute.name.startsWith("m_"))
                attribute.name = attribute.name.mid(2);

            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);

            qDebug() << "Member found:" << attribute.type << attribute.name;
        }

        QRegularExpression getterRegex(
            R"((?:^|\n)\s*([\w:<>]+)\s+(\w+)\s*\(\)\s*const\s*;)"
            );

        auto getterMatches = getterRegex.globalMatch(body);

        while (getterMatches.hasNext())
        {
            auto getterMatch = getterMatches.next();

            QString type = getterMatch.captured(1);
            QString name = getterMatch.captured(2);

            bool alreadyExists = false;

            for (const ParsedAttribute& attr : parsedClass.attributes)
            {
                if (attr.name == name)
                {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists)
            {
                ParsedAttribute attribute;
                attribute.type = type;
                attribute.name = name;
                attribute.isRelation = !isKnownPrimitiveType(type);

                parsedClass.attributes.append(attribute);

                qDebug() << "Getter found:" << attribute.type << attribute.name;
            }
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parseCsharpClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression classRegex(
        R"(class\s+(\w+)\s*\{([\s\S]*?)\})"
        );

    auto classMatches = classRegex.globalMatch(content);

    while (classMatches.hasNext())
    {
        auto match = classMatches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression propertyRegex(
            R"((public|private|protected)?\s*(string|int|double|float|bool|DateTime|\w+)\s+(\w+)\s*\{\s*get;\s*set;\s*\})"
            );

        auto propertyMatches = propertyRegex.globalMatch(body);

        while (propertyMatches.hasNext())
        {
            auto propertyMatch = propertyMatches.next();

            ParsedAttribute attribute;
            attribute.type = propertyMatch.captured(2);
            attribute.name = propertyMatch.captured(3);
            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parseCClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression structRegex(R"(typedef\s+struct\s*\{([\s\S]*?)\}\s*(\w+)\s*;|struct\s+(\w+)\s*\{([\s\S]*?)\};)");
    auto matches = structRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = !match.captured(2).isEmpty()
                               ? match.captured(2)
                               : match.captured(3);

        QString body = !match.captured(1).isEmpty()
                           ? match.captured(1)
                           : match.captured(4);

        QRegularExpression attrRegex(R"((int|double|float|bool|char|long|short)\s+(\w+)\s*;)");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.type = attrMatch.captured(1);
            attribute.name = attrMatch.captured(2);
            attribute.isRelation = false;

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parsePythonClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression classRegex(R"(class\s+(\w+).*?:([\s\S]*?)(?=\nclass\s+\w+|\z))");
    auto matches = classRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attrRegex(R"(self\.(\w+)\s*:\s*(\w+)|self\.(\w+)\s*=)");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.name = !attrMatch.captured(1).isEmpty()
                                 ? attrMatch.captured(1)
                                 : attrMatch.captured(3);

            attribute.type = !attrMatch.captured(2).isEmpty()
                                 ? attrMatch.captured(2)
                                 : "unknown";

            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parseGoClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression classRegex(R"(type\s+(\w+)\s+struct\s*\{([\s\S]*?)\})");
    auto matches = classRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attrRegex(R"((\w+)\s+(\w+))");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.name = attrMatch.captured(1);
            attribute.type = attrMatch.captured(2);
            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parseRustClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression structRegex(R"(struct\s+(\w+)\s*\{([\s\S]*?)\})");
    auto matches = structRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attrRegex(R"((\w+)\s*:\s*([\w:<>]+)\s*,?)");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.name = attrMatch.captured(1);
            attribute.type = attrMatch.captured(2);
            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parseFsharpClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression typeRegex(R"(type\s+(\w+)\s*=\s*\{([\s\S]*?)\})");
    auto matches = typeRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attrRegex(R"((\w+)\s*:\s*(\w+))");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.name = attrMatch.captured(1);
            attribute.type = attrMatch.captured(2);
            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parsePowershellClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression classRegex(R"(class\s+(\w+)\s*\{([\s\S]*?)\})");
    auto matches = classRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attrRegex(R"(\[(\w+)\]\s*\$(\w+))");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.type = attrMatch.captured(1);
            attribute.name = attrMatch.captured(2);
            attribute.isRelation = !isKnownPrimitiveType(attribute.type);

            parsedClass.attributes.append(attribute);
        }

        classes.append(parsedClass);
    }

    return classes;
}

QList<ParsedClass> ClassParser::parseJavaClasses(const QString& content)
{
    QList<ParsedClass> classes;

    QRegularExpression classRegex(R"(class\s+(\w+)\s*\{([\s\S]*?)\})");
    auto matches = classRegex.globalMatch(content);

    while (matches.hasNext())
    {
        auto match = matches.next();

        ParsedClass parsedClass;
        parsedClass.name = match.captured(1);

        QString body = match.captured(2);

        QRegularExpression attrRegex(R"((private|public|protected)?\s*(String|int|double|float|boolean|Date|LocalDate|LocalDateTime|\w+)\s+(\w+)\s*;)");
        auto attrs = attrRegex.globalMatch(body);

        while (attrs.hasNext())
        {
            auto attrMatch = attrs.next();

            ParsedAttribute attribute;
            attribute.type = attrMatch.captured(2);
            attribute.name = attrMatch.captured(3);
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
           || type == "boolean"
           || type == "QString"
           || type == "std::string"
           || type == "string"
           || type == "String"
           || type == "char"
           || type == "long"
           || type == "short"
           || type == "QDate"
           || type == "QDateTime"
           || type == "DateTime"
           || type == "Date"
           || type == "LocalDate"
           || type == "LocalDateTime"
           || type == "i32"
           || type == "i64"
           || type == "f32"
           || type == "f64"
           || type == "String"
           || type == "str";
}
