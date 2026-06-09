#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "utils/dalfileexporter.h"

class DalGenerationTest : public QObject
{
    Q_OBJECT

private slots:
    void exportsGeneratedDalFiles();
    void rejectsUnsafeGeneratedFileNames();
    void reportsMissingOutputPath();
};

void DalGenerationTest::exportsGeneratedDalFiles()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString response =
        "FILE: CustomerRepository.h\n"
        "#pragma once\n"
        "class CustomerRepository {};\n\n"
        "FILE: CustomerRepository.cpp\n"
        "#include \"CustomerRepository.h\"\n";

    const DalExportResult result =
        DalFileExporter::exportFiles(
            response,
            tempDir.path());

    QVERIFY(result.success());
    QCOMPARE(result.filesSaved, 2);
    QCOMPARE(result.message, QString("DAL files saved: 2"));

    QFile headerFile(
        QDir(tempDir.path()).filePath("CustomerRepository.h"));
    QVERIFY(headerFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(QString::fromUtf8(headerFile.readAll())
                .contains("CustomerRepository"));
}

void DalGenerationTest::rejectsUnsafeGeneratedFileNames()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString response =
        "FILE: ../BadRepository.h\n"
        "bad\n\n"
        "FILE: C:\\BadRepository.cpp\n"
        "bad\n";

    const DalExportResult result =
        DalFileExporter::exportFiles(
            response,
            tempDir.path());

    QVERIFY(!result.success());
    QCOMPARE(result.filesSaved, 0);
    QCOMPARE(
        result.message,
        QString("No valid DAL files were found in the AI response."));
}

void DalGenerationTest::reportsMissingOutputPath()
{
    const DalExportResult result =
        DalFileExporter::exportFiles(
            "FILE: CustomerRepository.h\nclass CustomerRepository {};",
            "");

    QVERIFY(!result.success());
    QCOMPARE(result.message, QString("No output path selected."));
}

QTEST_MAIN(DalGenerationTest)
#include "dalgenerationtest.moc"
