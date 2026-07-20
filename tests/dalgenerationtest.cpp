#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "utils/codegenerationprofile.h"
#include "utils/dalfileexporter.h"

class DalGenerationTest : public QObject
{
    Q_OBJECT

private slots:
    void appliesSqlTableNamingConvention();
    void exportsGeneratedDalFiles();
    void rejectsUnsafeGeneratedFileNames();
    void reportsMissingOutputPath();
    void providesProfileForEveryLanguage_data();
    void providesProfileForEveryLanguage();
    void adaptsSettingsToLanguageCapabilities();
    void acceptsSecureBindingForEveryLanguage_data();
    void acceptsSecureBindingForEveryLanguage();
    void validatesCompleteLayeredCode();
    void rejectsUnsafeOrWrongLanguageCode();
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

void DalGenerationTest::providesProfileForEveryLanguage_data()
{
    QTest::addColumn<int>("languageValue");
    QTest::addColumn<QString>("extension");

    using Type = ProgrammingLanguage::ProgrammingLanguageType;
    QTest::newRow("c++") << static_cast<int>(Type::Cplusplus) << "cpp";
    QTest::newRow("c") << static_cast<int>(Type::C) << "c";
    QTest::newRow("csharp") << static_cast<int>(Type::Csharp) << "cs";
    QTest::newRow("python") << static_cast<int>(Type::Python) << "py";
    QTest::newRow("go") << static_cast<int>(Type::Go) << "go";
    QTest::newRow("rust") << static_cast<int>(Type::Rust) << "rs";
    QTest::newRow("fsharp") << static_cast<int>(Type::Fsharp) << "fs";
    QTest::newRow("powershell") << static_cast<int>(Type::Powershell) << "ps1";
    QTest::newRow("java") << static_cast<int>(Type::Java) << "java";
}

void DalGenerationTest::providesProfileForEveryLanguage()
{
    QFETCH(int, languageValue);
    QFETCH(QString, extension);

    const auto language =
        static_cast<ProgrammingLanguage::ProgrammingLanguageType>(
            languageValue);
    const CodeGenerationOptions options =
        CodeGenerationProfile::optionsFor(language);
    const QVariantMap capabilities =
        CodeGenerationProfile::capabilities(language);

    const QString prompt =
        CodeGenerationProfile::buildPromptInstructions(
            language,
            "PostgreSQL",
            options);

    QVERIFY(!prompt.isEmpty());
    QVERIFY(prompt.contains(
        ProgrammingLanguage::languageTypeToText(language)));
    QVERIFY(!options.databaseApi.isEmpty());
    QVERIFY(prompt.contains(options.databaseApi));
    QVERIFY(!capabilities.value("architectures").toStringList().isEmpty());
    QVERIFY(!capabilities.value("databaseApis").toStringList().isEmpty());
    QVERIFY(prompt.contains("prepared statements"));
    QVERIFY(
        CodeGenerationProfile::allowedExtensions(language)
            .contains(extension));
}

void DalGenerationTest::adaptsSettingsToLanguageCapabilities()
{
    using Type = ProgrammingLanguage::ProgrammingLanguageType;

    const CodeGenerationOptions cOptions =
        CodeGenerationProfile::optionsFor(
            Type::C,
            {
                {"architecture", "Hexagonal"},
                {"dataAccessPattern", "Repository"},
                {"databaseApi", "Dapper"},
                {"dto", true},
                {"controller", true},
                {"domainModel", true},
                {"interfaces", true},
                {"asyncOperations", true}
            });

    QCOMPARE(cOptions.architecture, QString("Procedural"));
    QCOMPARE(cOptions.dataAccessPattern, QString("DAO"));
    QCOMPARE(cOptions.databaseApi, QString("Native prepared API"));
    QVERIFY(!cOptions.dto);
    QVERIFY(!cOptions.controller);
    QVERIFY(!cOptions.domainModel);
    QVERIFY(!cOptions.interfaces);
    QVERIFY(!cOptions.asyncOperations);

    const CodeGenerationOptions csharpOptions =
        CodeGenerationProfile::optionsFor(
            Type::Csharp,
            {
                {"databaseApi", "Dapper"},
                {"asyncOperations", true}
            });
    QCOMPARE(csharpOptions.databaseApi, QString("Dapper"));
    QVERIFY(csharpOptions.asyncOperations);

    const CodeGenerationOptions powershellOptions =
        CodeGenerationProfile::optionsFor(
            Type::Powershell,
            {
                {"architecture", "Script Module"},
                {"dataAccessPattern", "Functions"}
            });
    QCOMPARE(powershellOptions.architecture, QString("Script Module"));
    QCOMPARE(powershellOptions.dataAccessPattern, QString("Functions"));
    QVERIFY(!powershellOptions.interfaces);
    QVERIFY(!powershellOptions.asyncOperations);
}

void DalGenerationTest::acceptsSecureBindingForEveryLanguage_data()
{
    QTest::addColumn<int>("languageValue");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("code");

    using Type = ProgrammingLanguage::ProgrammingLanguageType;
    QTest::newRow("c++")
        << static_cast<int>(Type::Cplusplus)
        << "SqlUser.cpp"
        << "query.prepare(\"SELECT id FROM User WHERE id = :id\"); query.bindValue(\":id\", id);";
    QTest::newRow("c")
        << static_cast<int>(Type::C)
        << "SqlUser.c"
        << "sqlite3_prepare_v2(db, sql, -1, &stmt, 0); sqlite3_bind_int(stmt, 1, id);";
    QTest::newRow("csharp")
        << static_cast<int>(Type::Csharp)
        << "SqlUser.cs"
        << "command.Parameters.Add(parameter);";
    QTest::newRow("python")
        << static_cast<int>(Type::Python)
        << "SqlUser.py"
        << "cursor.execute(sql, (user_id,))";
    QTest::newRow("go")
        << static_cast<int>(Type::Go)
        << "SqlUser.go"
        << "db.QueryContext(ctx, query, id)";
    QTest::newRow("rust")
        << static_cast<int>(Type::Rust)
        << "SqlUser.rs"
        << "sqlx::query(query).bind(id).fetch_one(pool).await";
    QTest::newRow("fsharp")
        << static_cast<int>(Type::Fsharp)
        << "SqlUser.fs"
        << "command.Parameters.Add(parameter) |> ignore";
    QTest::newRow("powershell")
        << static_cast<int>(Type::Powershell)
        << "SqlUser.ps1"
        << "$command.Parameters.Add($parameter) | Out-Null";
    QTest::newRow("java")
        << static_cast<int>(Type::Java)
        << "SqlUser.java"
        << "PreparedStatement statement; statement.setInt(1, id);";
}

void DalGenerationTest::acceptsSecureBindingForEveryLanguage()
{
    QFETCH(int, languageValue);
    QFETCH(QString, fileName);
    QFETCH(QString, code);

    CodeGenerationOptions options;
    options.entity = false;
    options.dto = false;
    options.interfaces = false;

    const QString response =
        "FILE: " + fileName + "\n" + code;
    const QStringList errors =
        CodeGenerationProfile::validateResponse(
            response,
            static_cast<ProgrammingLanguage::ProgrammingLanguageType>(
                languageValue),
            options,
            {"User"});

    QVERIFY2(
        errors.isEmpty(),
        qPrintable(errors.join("\n")));
}

void DalGenerationTest::validatesCompleteLayeredCode()
{
    CodeGenerationOptions options;
    options.interfaces = false;

    const QString response =
        "FILE: UserEntity.h\n"
        "struct UserEntity { int id; };\n\n"
        "FILE: UserEntity.cpp\n"
        "#include \"UserEntity.h\"\n\n"
        "FILE: UserDto.h\n"
        "struct UserDto { int id; };\n\n"
        "FILE: UserDto.cpp\n"
        "#include \"UserDto.h\"\n\n"
        "FILE: SqlUser.h\n"
        "class SqlUser {};\n\n"
        "FILE: SqlUser.cpp\n"
        "QSqlQuery query;\n"
        "query.prepare(\"SELECT id FROM User WHERE id = :id\");\n"
        "query.bindValue(\":id\", id);\n\n"
        "FILE: dependencies.txt\n"
        "Qt 6.5\n";

    const QStringList errors =
        CodeGenerationProfile::validateResponse(
            response,
            ProgrammingLanguage::ProgrammingLanguageType::Cplusplus,
            options,
            {"User"});

    QVERIFY2(
        errors.isEmpty(),
        qPrintable(errors.join("\n")));
}

void DalGenerationTest::rejectsUnsafeOrWrongLanguageCode()
{
    CodeGenerationOptions options;
    options.entity = false;
    options.dto = false;
    options.interfaces = false;

    const QString response =
        "FILE: SqlUser.cs\n"
        "cursor.execute(f\"SELECT * FROM User WHERE id = {user_id}\")\n";

    const QStringList errors =
        CodeGenerationProfile::validateResponse(
            response,
            ProgrammingLanguage::ProgrammingLanguageType::Python,
            options,
            {"User"});

    QVERIFY(!errors.isEmpty());
    QVERIFY(errors.join("\n").contains("Unexpected file extension"));
    QVERIFY(errors.join("\n").contains("parameter binding"));
    QVERIFY(errors.join("\n").contains("string formatting"));
}

QTEST_MAIN(DalGenerationTest)
#include "dalgenerationtest.moc"
