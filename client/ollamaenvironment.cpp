#include "ollamaenvironment.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QUrl>

QString OllamaEnvironment::defaultEndpoint()
{
    return "http://localhost:11434";
}

bool OllamaEnvironment::isEndpointValid(
    const QString& endpoint)
{
    const QUrl url(endpoint.trimmed());

    return url.isValid()
           && !url.scheme().isEmpty()
           && !url.host().isEmpty()
           && (url.scheme() == "http" || url.scheme() == "https");
}

OllamaInstallationStatus OllamaEnvironment::detectInstallation(
    const QStringList& candidatePaths)
{
    QStringList paths = candidatePaths;

#ifdef Q_OS_WIN
    const QProcessEnvironment environment =
        QProcessEnvironment::systemEnvironment();

    const QString localAppData =
        environment.value("LOCALAPPDATA");

    if (!localAppData.isEmpty())
    {
        paths.append(
            QDir(localAppData)
                .filePath("Programs/Ollama/ollama.exe"));
    }

    paths.append("C:/Program Files/Ollama/ollama.exe");
#endif

    for (const QString& path : paths)
    {
        const QFileInfo fileInfo(path);

        if (fileInfo.exists() && fileInfo.isFile())
        {
            return {
                true,
                fileInfo.absoluteFilePath()
            };
        }
    }

    const QString executable =
        QStandardPaths::findExecutable("ollama");

    if (!executable.isEmpty())
    {
        return {
            true,
            executable
        };
    }

#ifdef Q_OS_WIN
    const QString windowsExecutable =
        QStandardPaths::findExecutable("ollama.exe");

    if (!windowsExecutable.isEmpty())
    {
        return {
            true,
            windowsExecutable
        };
    }
#endif

    return {};
}

QString OllamaEnvironment::setupInstructions(
    bool installed,
    bool apiReachable,
    const QStringList& models)
{
    if (!installed && !apiReachable)
    {
        return
            "Ollama is not installed or the configured endpoint is not reachable. "
            "Install Ollama from https://ollama.com/download, restart ProteusManager, "
            "then install a model with: ollama pull llama3.2";
    }

    if (!apiReachable)
    {
        return
            "Ollama is installed, but the API is not reachable. "
            "Start Ollama or run: ollama serve";
    }

    if (models.isEmpty())
    {
        return
            "Ollama is running, but no AI model is installed. "
            "Install one with: ollama pull llama3.2";
    }

    return "AI environment is ready.";
}
