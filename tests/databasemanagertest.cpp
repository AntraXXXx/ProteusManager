#include <QtTest>

#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QUuid>

#include "database/databasemanager.h"

namespace
{
QString createConnectionName()
{
    return "proteus_test_"
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

class DatabaseManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void startsDisconnected();
    void validatesSqlByStatementType();
    void opensDatabaseAndIntrospectsSchema();
};

void DatabaseManagerTest::initTestCase()
{
    if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
        QSKIP("QSQLITE driver is not available.");
}

void DatabaseManagerTest::startsDisconnected()
{
    DatabaseManager manager;
    QVERIFY(!manager.isConnected());
}

void DatabaseManagerTest::validatesSqlByStatementType()
{
    DatabaseManager manager;

    QVERIFY(manager.isValidSql(
        "CREATE TABLE Customer (id INTEGER PRIMARY KEY);"));
    QVERIFY(manager.isValidSql(
        "ALTER TABLE Customer ADD COLUMN name TEXT;"));
    QVERIFY(manager.isValidSql(
        "CREATE INDEX idx_customer_name ON Customer(name);"));
    QVERIFY(!manager.isValidSql(
        "SELECT * FROM Customer;"));
}

void DatabaseManagerTest::opensDatabaseAndIntrospectsSchema()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath = tempDir.filePath("proteus-test.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));
        QVERIFY(manager.isConnected());
        QCOMPARE(manager.getSqlConnectionName(), connectionName);

        QVERIFY(manager.executeQuery(
            "CREATE TABLE Customer ("
            "id INTEGER PRIMARY KEY, "
            "username TEXT"
            ");"
            "INSERT INTO Customer (username) VALUES ('akw');"));

        QVERIFY(manager.tableExists("Customer"));
        QVERIFY(manager.columnExists("Customer", "USERNAME"));
        QVERIFY(manager.hasRows("Customer"));

        const QStringList columns =
            manager.getColumnNames("Customer");
        QCOMPARE(columns, QStringList({"id", "username"}));

        const QString schema =
            manager.buildSchemaDescription();
        QVERIFY(schema.contains("Table: Customer"));
        QVERIFY(schema.contains("- username TEXT"));
    }

    removeConnection(connectionName);
}

QTEST_MAIN(DatabaseManagerTest)
#include "databasemanagertest.moc"
