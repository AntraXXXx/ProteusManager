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
};
#endif


