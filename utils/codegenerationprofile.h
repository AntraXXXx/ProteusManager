#ifndef CODEGENERATIONPROFILE_H
#define CODEGENERATIONPROFILE_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "programminglanguage.h"

struct CodeGenerationOptions
{
    bool entity = true;
    bool dto = true;
    bool repository = true;
    bool service = false;
    bool controller = false;
    bool domainModel = false;
    bool interfaces = true;
    bool unitTests = false;
    bool asyncOperations = false;
    QString architecture = "Layered";
    QString dataAccessPattern = "Repository";

    static CodeGenerationOptions fromVariantMap(
        const QVariantMap& values);

    QVariantMap toVariantMap() const;
    QStringList requestedLayers() const;
};

class CodeGenerationProfile
{
public:
    static QString buildPromptInstructions(
        ProgrammingLanguage::ProgrammingLanguageType language,
        const QString& databaseDialect,
        const CodeGenerationOptions& options);

    static QString generationStatus(
        ProgrammingLanguage::ProgrammingLanguageType language,
        const CodeGenerationOptions& options);

    static QStringList allowedExtensions(
        ProgrammingLanguage::ProgrammingLanguageType language);

    static QStringList validateResponse(
        const QString& response,
        ProgrammingLanguage::ProgrammingLanguageType language,
        const CodeGenerationOptions& options,
        const QStringList& tableNames);
};

#endif
