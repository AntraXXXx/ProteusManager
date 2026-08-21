#include <QtTest>

#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QUuid>

#include "database/databasemanager.h"
#include "database/sqlsafetypolicy.h"

namespace
{
QString createConnectionName()
{
    return "proteus_sql_validation_"
           + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void removeConnection(const QString& connectionName)
{
    if (!QSqlDatabase::contains(connectionName))
        return;

    {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen())
            db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
}
}

class SqlValidationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void acceptsSchemaStatements();
    void rejectsNonSchemaOrDestructiveStatements();
    void validatesMigrationPolicyAndDialect();
    void normalizesSqlStatements();
    void executesValidatedSchemaStatements();
};

void SqlValidationTest::initTestCase()
{
    if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
        QSKIP("QSQLITE driver is not available.");
}

void SqlValidationTest::acceptsSchemaStatements()
{
    DatabaseManager manager;

    QVERIFY(manager.isValidSql(
        "CREATE TABLE Customer (id INTEGER PRIMARY KEY);"));

    QVERIFY(manager.isValidSql(
        "CREATE TABLE IF NOT EXISTS Customer (id INTEGER PRIMARY KEY);"
        "ALTER TABLE Customer ADD COLUMN username TEXT;"
        "CREATE INDEX idx_customer_username ON Customer(username);"
        "CREATE UNIQUE INDEX idx_customer_id ON Customer(id);"));
}

void SqlValidationTest::rejectsNonSchemaOrDestructiveStatements()
{
    DatabaseManager manager;

    QVERIFY(!manager.isValidSql(""));
    QVERIFY(!manager.isValidSql("Here is the SQL: CREATE TABLE Customer (id INTEGER);"));
    QVERIFY(!manager.isValidSql("SELECT * FROM Customer;"));
    QVERIFY(!manager.isValidSql("DROP TABLE Customer;"));
    QVERIFY(!manager.isValidSql("CREATE TABLE Customer (id INTEGER); DROP TABLE Customer;"));
    QVERIFY(!manager.isValidSql("ALTER TABLE Customer RENAME TO Person;"));
    QVERIFY(!manager.isValidSql("INSERT INTO Customer (id) VALUES (1);"));
}

void SqlValidationTest::validatesMigrationPolicyAndDialect()
{
    QVERIFY(SqlSafetyPolicy::isValidMigrationSql(
        "CREATE TABLE CustomerCopy (id INTEGER PRIMARY KEY);"
        "INSERT INTO CustomerCopy SELECT id FROM Customer;"));
    QVERIFY(!SqlSafetyPolicy::isValidMigrationSql(
        "DROP TABLE Customer;"));
    QCOMPARE(
        SqlSafetyPolicy::incompatibleFeature(
            "QSQLITE",
            "SELECT CHARINDEX(',', value) FROM Source;"),
        QString("CHARINDEX("));
    QVERIFY(SqlSafetyPolicy::incompatibleFeature(
        "QSQLITE",
        "SELECT INSTR(value, ',') FROM Source;")
        .isEmpty());
    QCOMPARE(
        SqlSafetyPolicy::incompatibleFeature(
            "QPSQL",
            "SELECT IFNULL(name, '') FROM Customer;"),
        QString("IFNULL("));
    QCOMPARE(
        SqlSafetyPolicy::incompatibleFeature(
            "QMYSQL",
            "SELECT SPLIT_PART(value, ',', 1) FROM Source;"),
        QString("SPLIT_PART("));
    QCOMPARE(
        SqlSafetyPolicy::incompatibleFeature(
            "QODBC",
            "SELECT id FROM Customer LIMIT 10;"),
        QString("LIMIT 10"));
}

void SqlValidationTest::normalizesSqlStatements()
{
    QCOMPARE(
        SqlSafetyPolicy::splitStatements(
            "CREATE TABLE A (id INT); CREATE INDEX idx_a ON A(id);"),
        QStringList({
            "CREATE TABLE A (id INT)",
            " CREATE INDEX idx_a ON A(id)"
        }));
    QCOMPARE(
        SqlSafetyPolicy::stripLeadingComments(
            "-- generated migration\n"
            "-- validated by preview\n"
            "CREATE TABLE A (id INT)"),
        QString("CREATE TABLE A (id INT)"));
}

void SqlValidationTest::executesValidatedSchemaStatements()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("sql-validation.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));

        const QString sql =
            "CREATE TABLE Customer (id INTEGER PRIMARY KEY);"
            "ALTER TABLE Customer ADD COLUMN username TEXT;"
            "CREATE INDEX idx_customer_username ON Customer(username);";

        QVERIFY(manager.isValidSql(sql));
        QVERIFY(manager.executeQuery(sql));
        QVERIFY(manager.tableExists("Customer"));
        QVERIFY(manager.columnExists("Customer", "username"));
    }

    removeConnection(connectionName);
}

QTEST_MAIN(SqlValidationTest)
#include "sqlvalidationtest.moc"
