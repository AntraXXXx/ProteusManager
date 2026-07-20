#include <QtTest>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QVariantMap>

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

QVariantMap findTable(
    const QVariantList& schema,
    const QString& tableName)
{
    for (const QVariant& tableValue : schema)
    {
        const QVariantMap table = tableValue.toMap();
        if (table.value("name").toString().compare(
                tableName,
                Qt::CaseInsensitive) == 0)
        {
            return table;
        }
    }

    return {};
}

}

class DatabaseManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void startsDisconnected();
    void validatesSqlByStatementType();
    void validatesLosslessMigrationSql();
    void executesMigrationTransactionally();
    void executesCteMigrationTransactionally();
    void opensDatabaseAndIntrospectsSchema();
    void buildsSchemaDiagramsWithRelations();
    void buildsLanguageNeutralNormalizationAnalysis();
    void detectsNumberedNormalizationEvidence();
    void previewsGermanOrdersOnIsolatedCopy();
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

void DatabaseManagerTest::validatesLosslessMigrationSql()
{
    DatabaseManager manager;

    QVERIFY(manager.isValidMigrationSql(
        "CREATE TABLE CustomerAddress ("
        "customerId INTEGER, city TEXT);"
        "INSERT INTO CustomerAddress (customerId, city) "
        "SELECT id, city FROM Customer;"));
    QVERIFY(manager.isValidMigrationSql(
        "CREATE INDEX idx_customer_address "
        "ON CustomerAddress(customerId);"));

    QVERIFY(!manager.isValidMigrationSql(
        "DROP TABLE Customer;"));
    QVERIFY(!manager.isValidMigrationSql(
        "DELETE FROM Customer;"));
    QVERIFY(!manager.isValidMigrationSql(
        "UPDATE Customer SET city = NULL;"));
    QVERIFY(!manager.isValidMigrationSql(
        "INSERT INTO CustomerAddress VALUES (1, 'Berlin');"));
}

void DatabaseManagerTest::executesMigrationTransactionally()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("migration-test.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(
            connectionName,
            databasePath));
        QVERIFY(manager.executeQuery(
            "CREATE TABLE Customer ("
            "id INTEGER PRIMARY KEY, city TEXT);"
            "INSERT INTO Customer (id, city) VALUES (1, 'Berlin');"));

        const QString migration =
            "CREATE TABLE CustomerAddress ("
            "customerId INTEGER, city TEXT);"
            "INSERT INTO CustomerAddress (customerId, city) "
            "SELECT id, city FROM Customer;";

        QVERIFY(manager.executeMigration(migration));
        QVERIFY(manager.tableExists("Customer"));
        QVERIFY(manager.hasRows("Customer"));
        QVERIFY(manager.tableExists("CustomerAddress"));
        QVERIFY(manager.hasRows("CustomerAddress"));

        const QString failingMigration =
            "CREATE TABLE BrokenCopy (id INTEGER);"
            "INSERT INTO BrokenCopy (id) "
            "SELECT missingColumn FROM Customer;";

        QVERIFY(!manager.executeMigration(failingMigration));
        QVERIFY(!manager.tableExists("BrokenCopy"));
        QVERIFY(manager.hasRows("Customer"));
    }

    removeConnection(connectionName);
}

void DatabaseManagerTest::executesCteMigrationTransactionally()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("cte-migration-test.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));
        QVERIFY(manager.executeQuery(
            "CREATE TABLE SourceValues (valueList TEXT);"
            "INSERT INTO SourceValues VALUES ('one,two');"));

        const QString migration =
            "CREATE TABLE AtomicValues (value TEXT PRIMARY KEY);"
            "WITH RECURSIVE parts(value, rest) AS ("
            "SELECT TRIM(SUBSTR(valueList || ',', 1, "
            "INSTR(valueList || ',', ',') - 1)), "
            "SUBSTR(valueList || ',', INSTR(valueList || ',', ',') + 1) "
            "FROM SourceValues UNION ALL "
            "SELECT TRIM(SUBSTR(rest, 1, INSTR(rest, ',') - 1)), "
            "SUBSTR(rest, INSTR(rest, ',') + 1) "
            "FROM parts WHERE rest <> ''"
            ") INSERT INTO AtomicValues(value) "
            "SELECT value FROM parts WHERE value <> '';";

        QVERIFY(manager.isValidMigrationSql(migration));
        QVERIFY2(
            manager.validateMigrationPreview(migration),
            qPrintable(manager.lastError()));
        QVERIFY(!manager.tableExists("AtomicValues"));
        QVERIFY(manager.hasRows("SourceValues"));

        const QString incompatibleMigration =
            "CREATE TABLE InvalidDialect (position INTEGER);"
            "INSERT INTO InvalidDialect(position) "
            "SELECT CHARINDEX(',', valueList) FROM SourceValues;";
        QVERIFY(manager.isValidMigrationSql(incompatibleMigration));
        QVERIFY(!manager.validateMigrationPreview(incompatibleMigration));
        QVERIFY(manager.lastError().contains("incompatible"));
        QVERIFY(!manager.tableExists("InvalidDialect"));

        QVERIFY2(
            manager.executeMigration(migration),
            qPrintable(manager.lastError()));
        QVERIFY(manager.tableExists("AtomicValues"));
        QVERIFY(manager.hasRows("AtomicValues"));
        QVERIFY(manager.hasRows("SourceValues"));

        const QVariantList appliedDiagram =
            manager.buildSchemaDiagramWithMigration(migration);
        QCOMPARE(appliedDiagram.size(), 1);
        const QVariantMap atomicTable =
            findTable(appliedDiagram, "AtomicValues");
        QVERIFY(!atomicTable.isEmpty());
        QVERIFY(!atomicTable.value("proposed").toBool());
    }

    removeConnection(connectionName);
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

void DatabaseManagerTest::buildsSchemaDiagramsWithRelations()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("diagram-test.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));
        QVERIFY(manager.executeQuery(
            "PRAGMA foreign_keys = ON;"
            "CREATE TABLE Customer ("
            "id INTEGER PRIMARY KEY, name TEXT);"
            "CREATE TABLE Address ("
            "id INTEGER PRIMARY KEY, customerId INTEGER, city TEXT, "
            "FOREIGN KEY(customerId) REFERENCES Customer(id));"));

        const QVariantList before =
            manager.buildSchemaDiagram();
        const QVariantMap customer =
            findTable(before, "Customer");
        const QVariantMap address =
            findTable(before, "Address");
        QVERIFY(!customer.isEmpty());
        QVERIFY(!address.isEmpty());
        QCOMPARE(address.value("relations").toList().size(), 1);

        const QVariantMap addressRelation =
            address.value("relations").toList().first().toMap();
        QCOMPARE(
            addressRelation.value("referenceTable").toString(),
            QString("Customer"));
        QCOMPARE(
            addressRelation.value("referenceColumn").toString(),
            QString("id"));

        const QString migration =
            "CREATE TABLE CustomerPhone ("
            "id INTEGER PRIMARY KEY, customerId INTEGER, phone TEXT, "
            "FOREIGN KEY(customerId) REFERENCES Customer(id));";
        const QVariantList after =
            manager.buildSchemaDiagramWithMigration(migration);
        QVERIFY(findTable(after, "Customer").isEmpty());
        const QVariantMap customerPhone =
            findTable(after, "CustomerPhone");
        QVERIFY(!customerPhone.isEmpty());
        QVERIFY(customerPhone.value("proposed").toBool());
        QCOMPARE(
            customerPhone.value("relations").toList().size(),
            1);
    }

    removeConnection(connectionName);
}

void DatabaseManagerTest::buildsLanguageNeutralNormalizationAnalysis()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("normalization-test.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));
        QVERIFY(manager.executeQuery(
            "CREATE TABLE Bestellungen_Unnormalisiert ("
            "BestellID VARCHAR(10), Kundennummer VARCHAR(10), "
            "KundenName VARCHAR(100), KundenAdresse VARCHAR(255), "
            "Bestelldatum DATE, ArtikelIDs VARCHAR(100), "
            "ArtikelNamen VARCHAR(255), Mengen VARCHAR(50), "
            "Einzelpreise VARCHAR(100), Gesamtbetrag DECIMAL(10, 2), "
            "Bezahlt VARCHAR(5), Zahlungsart VARCHAR(50));"
            "INSERT INTO Bestellungen_Unnormalisiert VALUES "
            "('B1001', 'K901', 'Max Mustermann', 'Hauptstr. 5, Berlin', "
            "'2026-06-20', 'A50, A72', 'Laptop, Maus', '1, 2', "
            "'999.00, 25.00', 1049.00, 'Ja', 'PayPal'),"
            "('B1002', 'K902', 'Anna Schmidt', 'Waldweg 12, Koeln', "
            "'2026-06-21', 'A80', 'Tastatur', '1', '45.00', "
            "45.00, 'Nein', 'Rechnung'),"
            "('B1003', 'K901', 'Max Mustermann', 'Hauptstr. 5, Berlin', "
            "'2026-06-22', 'A72, A90', 'Maus, Monitor', '1, 1', "
            "'25.00, 250.00', 275.00, 'Ja', 'PayPal');"
            "CREATE TABLE OrderDraft ("
            "OrderId VARCHAR(10), ProductCodes VARCHAR(100), "
            "Quantities VARCHAR(50));"
            "INSERT INTO OrderDraft VALUES "
            "('O-1', 'P-10|P-20', '1|3');"));

        const QString analysis =
            manager.buildNormalizationAnalysis({}, 5);

        QVERIFY(analysis.contains("Table: Bestellungen_Unnormalisiert"));
        QVERIFY(analysis.contains("ArtikelIDs='A50, A72'"));
        QVERIFY(analysis.contains("column ArtikelIDs: delimiter=comma"));
        QVERIFY(analysis.contains("Table: OrderDraft"));
        QVERIFY(analysis.contains("ProductCodes='P-10|P-20'"));
        QVERIFY(analysis.contains("column ProductCodes: delimiter=pipe"));
        QVERIFY(analysis.contains("infer their natural language"));

        const QString activeAnalysis =
            manager.buildNormalizationAnalysis(
                {"OrderDraft"},
                5);
        QVERIFY(activeAnalysis.contains("Table: OrderDraft"));
        QVERIFY(!activeAnalysis.contains("Bestellungen_Unnormalisiert"));

        const QVariantList activeDiagram =
            manager.buildSchemaDiagramForTables(
                {"OrderDraft"});
        QCOMPARE(activeDiagram.size(), 1);
        QCOMPARE(
            activeDiagram.first().toMap().value("name").toString(),
            QString("OrderDraft"));
        QVERIFY(!manager.tableExists("Bestellungen_1NF"));
        QVERIFY(!manager.tableExists("OrderItems"));
    }

    removeConnection(connectionName);
}

void DatabaseManagerTest::detectsNumberedNormalizationEvidence()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("numbered-normalization-test.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));
        QVERIFY(manager.executeQuery(
            "CREATE TABLE bestellungen_nicht_normalisiert ("
            "bestell_id INTEGER PRIMARY KEY, "
            "kunde_id INTEGER, kunde_name TEXT, "
            "produkt_1_name TEXT, produkt_1_menge INTEGER, "
            "produkt_2_name TEXT, produkt_2_menge INTEGER, "
            "produkt_3_name TEXT, produkt_3_menge INTEGER);"
            "INSERT INTO bestellungen_nicht_normalisiert VALUES "
            "(1, 10, 'Max Mustermann', 'Laptop', 1, 'Maus', 2, NULL, NULL);"));

        const QString analysis =
            manager.buildNormalizationAnalysis({}, 5);

        QVERIFY(analysis.contains(
            "Repeated numbered column group evidence"));
        QVERIFY(analysis.contains("prefix produkt"));
        QVERIFY(manager.hasNormalizationEvidence());
        QVERIFY(manager.hasNormalizationEvidence(
            {"bestellungen_nicht_normalisiert"}));
    }

    removeConnection(connectionName);
}

void DatabaseManagerTest::previewsGermanOrdersOnIsolatedCopy()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString connectionName = createConnectionName();
    const QString databasePath =
        tempDir.filePath("orders-normalization.sqlite");

    {
        DatabaseManager manager;
        QVERIFY(manager.openDatabase(connectionName, databasePath));
        QVERIFY(manager.executeQuery(
            "CREATE TABLE Bestellungen ("
            "BestellungID INT, Bestelldatum DATE, "
            "KundeName VARCHAR(100), KundeStrasse VARCHAR(100), "
            "KundeStadt VARCHAR(50), KundePLZ VARCHAR(10), "
            "ArtikelName VARCHAR(100), ArtikelKategorie VARCHAR(50), "
            "ArtikelPreis DECIMAL(10, 2), BestellteMenge INT, "
            "GesamtPositionspreis DECIMAL(10, 2));"
            "INSERT INTO Bestellungen VALUES "
            "(1, '2026-07-15', 'Max Mustermann', 'Hauptstrasse 1', "
            "'Hannover', '30159', 'Laptop', 'Elektronik', 1200.00, 1, 1200.00),"
            "(2, '2026-07-16', 'Anna Schmidt', 'Nebenstrasse 2', "
            "'Berlin', '10115', 'Kaffeemaschine', 'Haushalt', 150.00, 2, 300.00),"
            "(2, '2026-07-16', 'Anna Schmidt', 'Nebenstrasse 2', "
            "'Berlin', '10115', 'Kaffeebohnen', 'Kueche', 12.50, 5, 62.50);"));

        const QVariantList beforeSchema =
            manager.buildSchemaDiagram();
        QCOMPARE(beforeSchema.size(), 1);

        QSqlDatabase sourceDb =
            QSqlDatabase::database(connectionName);
        QSqlQuery rowCountQuery(
            "SELECT COUNT(*) FROM Bestellungen",
            sourceDb);
        QVERIFY(rowCountQuery.next());
        QCOMPARE(rowCountQuery.value(0).toInt(), 3);
        rowCountQuery.finish();

        const QString migration =
            "CREATE TABLE Bestellung_1NF ("
            "BestellungID INT, Bestelldatum DATE, "
            "KundeName VARCHAR(100), ArtikelName VARCHAR(100), "
            "BestellteMenge INT);"
            "INSERT INTO Bestellung_1NF "
            "SELECT BestellungID, Bestelldatum, KundeName, "
            "ArtikelName, BestellteMenge FROM Bestellungen;";

        QVERIFY2(
            manager.validateMigrationPreview(migration),
            qPrintable(manager.lastError()));
        QVERIFY(manager.tableExists("Bestellungen"));
        QVERIFY(!manager.tableExists("Bestellung_1NF"));
        QCOMPARE(manager.buildSchemaDiagram(), beforeSchema);

        QSqlQuery preservedRows(
            "SELECT COUNT(*) FROM Bestellungen",
            sourceDb);
        QVERIFY(preservedRows.next());
        QCOMPARE(preservedRows.value(0).toInt(), 3);
        preservedRows.finish();

        const QString failingMigration =
            "CREATE TABLE Broken_1NF (id INT);"
            "INSERT INTO Broken_1NF "
            "SELECT MissingColumn FROM Bestellungen;";
        QVERIFY(!manager.validateMigrationPreview(failingMigration));
        QVERIFY(manager.tableExists("Bestellungen"));
        QVERIFY(!manager.tableExists("Broken_1NF"));

        QSqlQuery rowsAfterFailure(
            "SELECT COUNT(*) FROM Bestellungen",
            sourceDb);
        QVERIFY(rowsAfterFailure.next());
        QCOMPARE(rowsAfterFailure.value(0).toInt(), 3);
    }

    removeConnection(connectionName);
}

QTEST_MAIN(DatabaseManagerTest)
#include "databasemanagertest.moc"
