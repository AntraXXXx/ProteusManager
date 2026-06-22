#include "programminglanguage.h"

#include <QHash>
#include <QRegularExpression>
#include <QStringList>

namespace
{
QString removeTypeNoise(QString type)
{
    type = type.trimmed();
    type.remove(QRegularExpression(R"(\bconst\b)"));
    type.remove(QRegularExpression(R"(\breadonly\b)"));
    type.remove(QRegularExpression(R"(\bmut\b)"));
    type.remove("&");
    type.remove("*");
    return type.trimmed();
}

QString unwrapGenericType(const QString& type)
{
    QRegularExpression genericRegex(R"(^[\w:.\[\]]+\s*[<\[]\s*([^<>\[\]]+)\s*[>\]]$)");
    QRegularExpressionMatch match = genericRegex.match(type.trimmed());

    if (match.hasMatch())
        return match.captured(1).trimmed();

    return type.trimmed();
}

QString normalizeKey(const QString& type)
{
    QString normalized =
        ProgrammingLanguage::normalizeType(type).toLower();

    normalized.remove("std::");
    normalized.remove("system.");
    normalized.remove("chrono::");
    normalized.replace("?", "");

    if (normalized.endsWith(" option"))
        normalized.chop(QString(" option").size());

    if (normalized.endsWith(" list"))
        normalized.chop(QString(" list").size());

    return normalized.trimmed();
}

bool isBinaryCollection(const QString& type)
{
    const QString compact =
        type.trimmed().toLower().remove(" ");

    return compact == "byte[]"
           || compact == "[]byte"
           || compact == "u8[]"
           || compact == "[]u8"
           || compact == "vec<u8>";
}

QHash<QString, QString> sqlTypeMap()
{
    return {
        {"bool", "BOOLEAN"},
        {"boolean", "BOOLEAN"},

        {"byte", "INTEGER"},
        {"sbyte", "INTEGER"},
        {"short", "INTEGER"},
        {"ushort", "INTEGER"},
        {"int", "INTEGER"},
        {"integer", "INTEGER"},
        {"uint", "INTEGER"},
        {"long", "INTEGER"},
        {"ulong", "INTEGER"},
        {"long long", "INTEGER"},
        {"int16", "INTEGER"},
        {"int32", "INTEGER"},
        {"int64", "INTEGER"},
        {"uint16", "INTEGER"},
        {"uint32", "INTEGER"},
        {"uint64", "INTEGER"},
        {"i8", "INTEGER"},
        {"i16", "INTEGER"},
        {"i32", "INTEGER"},
        {"i64", "INTEGER"},
        {"isize", "INTEGER"},
        {"u8", "INTEGER"},
        {"u16", "INTEGER"},
        {"u32", "INTEGER"},
        {"u64", "INTEGER"},
        {"usize", "INTEGER"},
        {"qint64", "INTEGER"},
        {"quint64", "INTEGER"},

        {"float", "REAL"},
        {"single", "REAL"},
        {"double", "REAL"},
        {"f32", "REAL"},
        {"f64", "REAL"},

        {"decimal", "NUMERIC"},
        {"bigdecimal", "NUMERIC"},

        {"char", "TEXT"},
        {"qchar", "TEXT"},
        {"str", "TEXT"},
        {"string", "TEXT"},
        {"std::string", "TEXT"},
        {"qstring", "TEXT"},
        {"uuid", "TEXT"},
        {"guid", "TEXT"},

        {"date", "DATE"},
        {"qdate", "DATE"},
        {"localdate", "DATE"},
        {"dateonly", "DATE"},
        {"naivedate", "DATE"},

        {"datetime", "DATETIME"},
        {"qdatetime", "DATETIME"},
        {"localdatetime", "DATETIME"},
        {"datetimeoffset", "DATETIME"},
        {"time.time", "DATETIME"},
        {"instant", "DATETIME"},
        {"naivedatetime", "DATETIME"},

        {"bytearray", "BLOB"},
        {"qbytearray", "BLOB"},
        {"bytes", "BLOB"},
        {"byte[]", "BLOB"},
        {"[]byte", "BLOB"}
    };
}
}

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

QString ProgrammingLanguage::normalizeType(const QString& type)
{
    QString normalized =
        removeTypeNoise(type);

    normalized.replace(QRegularExpression(R"(\s+)"), " ");

    if (normalized.endsWith("[]"))
        return normalized.left(normalized.size() - 2).trimmed();

    if (normalized.startsWith("[]"))
        return normalized.mid(2).trimmed();

    const QString unwrapped =
        unwrapGenericType(normalized);

    if (unwrapped != normalized)
        return normalizeType(unwrapped);

    return normalized;
}

QString ProgrammingLanguage::mapToSqlType(
    const QString& type,
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    Q_UNUSED(language);

    if (isBinaryCollection(type))
        return "BLOB";

    const QHash<QString, QString> mappings =
        sqlTypeMap();

    const QString key =
        normalizeKey(type);

    if (mappings.contains(key))
        return mappings.value(key);

    return "TEXT";
}

bool ProgrammingLanguage::isNullableType(const QString& type)
{
    const QString trimmed =
        type.trimmed();

    return trimmed.endsWith("?")
           || trimmed.contains(
               QRegularExpression(
                   R"(\b(nullable|optional|option)\s*[<\[])",
                   QRegularExpression::CaseInsensitiveOption))
           || trimmed.endsWith(" option", Qt::CaseInsensitive);
}

bool ProgrammingLanguage::isCollectionType(const QString& type)
{
    const QString trimmed =
        type.trimmed();

    return trimmed.startsWith("[]")
           || trimmed.endsWith("[]")
           || trimmed.endsWith(" list", Qt::CaseInsensitive)
           || trimmed.contains(
               QRegularExpression(
                   R"(\b(vector|list|array|qvector|qlist|ienumerable|icollection|hashset|vec|seq)\s*[<\[])",
                   QRegularExpression::CaseInsensitiveOption));
}

bool ProgrammingLanguage::isPrimitiveType(
    const QString& type,
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    Q_UNUSED(language);

    const QHash<QString, QString> mappings =
        sqlTypeMap();

    return mappings.contains(
        normalizeKey(type));
}

bool ProgrammingLanguage::isRelationshipType(
    const QString& type,
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    const QString normalized =
        normalizeType(type);

    if (normalized.compare("unknown", Qt::CaseInsensitive) == 0)
        return true;

    if (isCollectionType(type))
        return !isBinaryCollection(type);

    return !isPrimitiveType(
        normalized,
        language);
}
