#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "utils/dalfileexporter.h"

class DalGenerationTest : public QObject
{
    Q_OBJECT

private slots:
    void appliesSqlTableNamingConvention();
    void exportsGeneratedDalFiles();
    void rejectsUnsafeGeneratedFileNames();
    void reportsMissingOutputPath();
};

void DalGenerationTest::appliesSqlTableNamingConvention()
{
    const QString response =
        "FILE: userrepository.h\n"
        "#ifndef USERREPOSITORY_H\n"
        "#define USERREPOSITORY_H\n"
        "class QSqlQueryModel;\n"
        "class UserRepository {\n"
        "public:\n"
        "    UserRepository();\n"
        "};\n"
        "#endif // USERREPOSITORY_H\n\n"
        "FILE: userrepository.cpp\n"
        "#include \"userrepository.h\"\n"
        "UserRepository::UserRepository() {}\n"
        "const char* sql = \"INSERT INTO User (name) VALUES (:name)\";\n";

    const QString normalized =
        DalFileExporter::applySqlNamingConvention(
            response,
            {"User"});

    QVERIFY(normalized.contains("FILE: SqlUser.h"));
    QVERIFY(normalized.contains("FILE: SqlUser.cpp"));
    QVERIFY(normalized.contains("class SqlUser"));
    QVERIFY(normalized.contains("class QSqlQueryModel;"));
    QVERIFY(normalized.contains("SqlUser::SqlUser()"));
    QVERIFY(normalized.contains("#include \"SqlUser.h\""));
    QVERIFY(normalized.contains("SQLUSER_H"));
    QVERIFY(normalized.contains("INSERT INTO User"));
    QVERIFY(!normalized.contains("UserRepository"));

    const QString userList =
        DalFileExporter::applySqlNamingConvention(
            "FILE: UserListRepository.cs\n"
            "public class UserListRepository {}",
            {"UserList"});

    QVERIFY(userList.contains("FILE: SqlUserList.cs"));
    QVERIFY(userList.contains("class SqlUserList"));
}

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
