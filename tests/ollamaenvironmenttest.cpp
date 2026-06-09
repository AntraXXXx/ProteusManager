#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "client/ollamaenvironment.h"

class OllamaEnvironmentTest : public QObject
{
    Q_OBJECT

private slots:
    void validatesEndpointUrls();
    void detectsInstallationFromCandidatePath();
    void returnsSetupInstructionsForMissingStates();
};

void OllamaEnvironmentTest::validatesEndpointUrls()
{
    QVERIFY(OllamaEnvironment::isEndpointValid(
        "http://localhost:11434"));
    QVERIFY(OllamaEnvironment::isEndpointValid(
        "https://example.test:11434"));

    QVERIFY(!OllamaEnvironment::isEndpointValid(""));
    QVERIFY(!OllamaEnvironment::isEndpointValid("localhost:11434"));
    QVERIFY(!OllamaEnvironment::isEndpointValid("ftp://localhost"));
}

void OllamaEnvironmentTest::detectsInstallationFromCandidatePath()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

#ifdef Q_OS_WIN
    const QString executablePath =
        QDir(tempDir.path()).filePath("ollama.exe");
#else
    const QString executablePath =
        QDir(tempDir.path()).filePath("ollama");
#endif

    QFile file(executablePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("test");
    file.close();

    const OllamaInstallationStatus status =
        OllamaEnvironment::detectInstallation({executablePath});

    QVERIFY(status.installed);
    QCOMPARE(status.executablePath, executablePath);
}

void OllamaEnvironmentTest::returnsSetupInstructionsForMissingStates()
{
    QVERIFY(
        OllamaEnvironment::setupInstructions(
            false,
            false,
            {})
            .contains("Install Ollama"));

    QVERIFY(
        OllamaEnvironment::setupInstructions(
            true,
            false,
            {})
            .contains("ollama serve"));

    QVERIFY(
        OllamaEnvironment::setupInstructions(
            true,
            true,
            {})
            .contains("ollama pull"));

    QCOMPARE(
        OllamaEnvironment::setupInstructions(
            true,
            true,
            {"llama3.2"}),
        QString("AI environment is ready."));
}

QTEST_MAIN(OllamaEnvironmentTest)
#include "ollamaenvironmenttest.moc"
