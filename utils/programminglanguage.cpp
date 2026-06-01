#include "programminglanguage.h"

QString ProgrammingLanguage::languageTypeToText(
    ProgrammingLanguage::ProgrammingLanguageType type)
{
    switch (type)
    {
    case ProgrammingLanguageType::Cplusplus:
        return "C++";
    case ProgrammingLanguageType::C:
        return "C";
    case ProgrammingLanguageType::Csharp:
        return "C#";
    case ProgrammingLanguageType::Python:
        return "Python";
    case ProgrammingLanguageType::Go:
        return "Go";
    case ProgrammingLanguageType::Rust:
        return "Rust";
    case ProgrammingLanguageType::Fsharp:
        return "F#";
    case ProgrammingLanguageType::Powershell:
        return "PowerShell";
    case ProgrammingLanguageType::Java:
        return "Java";
    }

    return "Unknown";
}
