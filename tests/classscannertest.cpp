#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include "utils/classscanner.h"

namespace
{
bool writeTextFile(const QString& filePath, const QString& content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << content;
    return true;
}
}

class ClassScannerTest : public QObject
{
    Q_OBJECT

private slots:
    void returnsLanguageSpecificFileFilters();
    void scansClassFilesRecursively();
};

void ClassScannerTest::returnsLanguageSpecificFileFilters()
{
    ClassScanner scanner;

    QCOMPARE(
        scanner.fileFiltersForLanguage(
            ProgrammingLanguage::ProgrammingLanguageType::Cplusplus),
        QStringList({"*.h", "*.hpp"}));

    QCOMPARE(
        scanner.fileFiltersForLanguage(
            ProgrammingLanguage::ProgrammingLanguageType::Python),
        QStringList({"*.py"}));

    QCOMPARE(
        scanner.fileFiltersForLanguage(
            ProgrammingLanguage::ProgrammingLanguageType::Java),
        QStringList({"*.java"}));
}

void ClassScannerTest::scansClassFilesRecursively()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QDir root(tempDir.path());
    QVERIFY(root.mkpath("models/nested"));

    QVERIFY(writeTextFile(
        root.filePath("User.h"),
        "class User { int m_id; };"));

    QVERIFY(writeTextFile(
        root.filePath("models/nested/Order.hpp"),
        "class Order { int m_id; };"));

    QVERIFY(writeTextFile(
        root.filePath("models/nested/ignored.txt"),
        "class Ignored { int m_id; };"));

    ClassScanner scanner;
    const QList<ScannedClassFile> files =
        scanner.scanAndReadClassFiles(
            tempDir.path(),
            ProgrammingLanguage::ProgrammingLanguageType::Cplusplus);

    QCOMPARE(files.size(), 2);

    QStringList fileNames;
    for (const ScannedClassFile& file : files)
    {
        fileNames.append(QFileInfo(file.filePath).fileName());
        QVERIFY(!file.content.isEmpty());
    }

    fileNames.sort();
    QCOMPARE(fileNames, QStringList({"Order.hpp", "User.h"}));
}

QTEST_MAIN(ClassScannerTest)
#include "classscannertest.moc"
