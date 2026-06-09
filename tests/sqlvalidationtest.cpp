#include <QtTest>

#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QUuid>

#include "database/databasemanager.h"

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
