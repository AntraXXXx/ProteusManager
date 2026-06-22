#ifndef PROGRAMMINGLANGUAGE_H
#define PROGRAMMINGLANGUAGE_H

#include <QString>

class ProgrammingLanguage
{
public:
    enum class ProgrammingLanguageType
    {
        Cplusplus,
        C,
        Csharp,
        Python,
        Go,
        Rust,
        Fsharp,
        Powershell,
        Java
    };

    static QString languageTypeToText(ProgrammingLanguageType type);
    static QString normalizeType(const QString& type);
    static QString mapToSqlType(
        const QString& type,
        ProgrammingLanguageType language);
    static bool isNullableType(const QString& type);
    static bool isCollectionType(const QString& type);
    static bool isPrimitiveType(
        const QString& type,
        ProgrammingLanguageType language);
    static bool isRelationshipType(
        const QString& type,
        ProgrammingLanguageType language);
};
#endif


