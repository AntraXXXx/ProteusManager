#ifndef OLLAMAENVIRONMENT_H
#define OLLAMAENVIRONMENT_H

#include <QString>
#include <QStringList>

struct OllamaInstallationStatus
{
    bool installed = false;
    QString executablePath;
};

class OllamaEnvironment
{
public:
    static QString defaultEndpoint();
    static bool isEndpointValid(const QString& endpoint);

    static OllamaInstallationStatus detectInstallation(
        const QStringList& candidatePaths = {});

    static QString setupInstructions(
        bool installed,
        bool apiReachable,
        const QStringList& models);
};

#endif
