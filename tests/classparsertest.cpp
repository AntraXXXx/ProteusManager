#include <QtTest>

#include "utils/classparser.h"

namespace
{
const ParsedAttribute *findAttribute(
    const ParsedClass& parsedClass,
    const QString& name)
{
    for (const ParsedAttribute& attribute : parsedClass.attributes)
    {
        if (attribute.name == name)
            return &attribute;
    }

    return nullptr;
}
}

class ClassParserTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesCppMembersAndRelations();
    void parsesCStructFields();
    void parsesCsharpPropertiesAndRelations();
    void parsesPythonConstructorAttributes();
    void parsesGoStructFields();
    void parsesRustStructFields();
    void parsesFsharpRecordFields();
    void parsesPowershellClassProperties();
    void parsesJavaFields();
    void mapsLanguageTypesToSql();
    void returnsEmptyListForContentWithoutClasses();
};

void ClassParserTest::parsesCppMembersAndRelations()
{
    const QString source = R"(
class User {
public:
    QString username() const;

private:
    int m_id;
    QString m_username;
    Order m_order;
};
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Cplusplus);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("User"));

    const ParsedAttribute *id =
        findAttribute(classes.first(), "id");
    QVERIFY(id != nullptr);
    QCOMPARE(id->type, QString("int"));
    QVERIFY(!id->isRelation);

    const ParsedAttribute *username =
        findAttribute(classes.first(), "username");
    QVERIFY(username != nullptr);
    QCOMPARE(username->type, QString("QString"));
    QVERIFY(!username->isRelation);

    const ParsedAttribute *order =
        findAttribute(classes.first(), "order");
    QVERIFY(order != nullptr);
    QCOMPARE(order->type, QString("Order"));
    QVERIFY(order->isRelation);
}

void ClassParserTest::parsesCStructFields()
{
    const QString source = R"(
typedef struct {
    int id;
    char name[64];
    Customer* owner;
} Order;
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::C);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Order"));

    const ParsedAttribute *id =
        findAttribute(classes.first(), "id");
    QVERIFY(id != nullptr);
    QCOMPARE(id->type, QString("int"));
    QVERIFY(!id->isRelation);

    const ParsedAttribute *owner =
        findAttribute(classes.first(), "owner");
    QVERIFY(owner != nullptr);
    QCOMPARE(owner->type, QString("Customer*"));
    QVERIFY(owner->isRelation);
}

void ClassParserTest::parsesCsharpPropertiesAndRelations()
{
    const QString source = R"(
public class Invoice {
    public int? Id { get; set; }
    public string Number { get; set; }
    public List<Customer> Customers { get; set; }
}
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Csharp);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Invoice"));

    const ParsedAttribute *id =
        findAttribute(classes.first(), "Id");
    QVERIFY(id != nullptr);
    QCOMPARE(id->type, QString("int?"));
    QVERIFY(!id->isRelation);

    const ParsedAttribute *customers =
        findAttribute(classes.first(), "Customers");
    QVERIFY(customers != nullptr);
    QCOMPARE(customers->type, QString("List<Customer>"));
    QVERIFY(customers->isRelation);
}

void ClassParserTest::parsesPythonConstructorAttributes()
{
    const QString source = R"(
class Customer:
    def __init__(self):
        self.id: int = 0
        self.name: str = ""
        self.orders: list[Order] = []
        self.address = None
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Python);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Customer"));

    const ParsedAttribute *id =
        findAttribute(classes.first(), "id");
    QVERIFY(id != nullptr);
    QCOMPARE(id->type, QString("int"));
    QVERIFY(!id->isRelation);

    const ParsedAttribute *name =
        findAttribute(classes.first(), "name");
    QVERIFY(name != nullptr);
    QCOMPARE(name->type, QString("str"));
    QVERIFY(!name->isRelation);

    const ParsedAttribute *orders =
        findAttribute(classes.first(), "orders");
    QVERIFY(orders != nullptr);
    QCOMPARE(orders->type, QString("list[Order]"));
    QVERIFY(orders->isRelation);

    const ParsedAttribute *address =
        findAttribute(classes.first(), "address");
    QVERIFY(address != nullptr);
    QCOMPARE(address->type, QString("unknown"));
    QVERIFY(address->isRelation);
}

void ClassParserTest::parsesGoStructFields()
{
    const QString source = R"(
type Customer struct {
    ID int
    Name string
    Orders []Order
}
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Go);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Customer"));

    const ParsedAttribute *orders =
        findAttribute(classes.first(), "Orders");
    QVERIFY(orders != nullptr);
    QCOMPARE(orders->type, QString("[]Order"));
    QVERIFY(orders->isRelation);
}

void ClassParserTest::parsesRustStructFields()
{
    const QString source = R"(
struct Customer {
    pub id: i32,
    pub name: String,
    pub orders: Vec<Order>,
}
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Rust);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Customer"));

    const ParsedAttribute *orders =
        findAttribute(classes.first(), "orders");
    QVERIFY(orders != nullptr);
    QCOMPARE(orders->type, QString("Vec<Order>"));
    QVERIFY(orders->isRelation);
}

void ClassParserTest::parsesFsharpRecordFields()
{
    const QString source = R"(
type Customer = {
    Id: int
    Name: string
    Orders: Order list
}
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Fsharp);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Customer"));

    const ParsedAttribute *orders =
        findAttribute(classes.first(), "Orders");
    QVERIFY(orders != nullptr);
    QCOMPARE(orders->type, QString("Order list"));
    QVERIFY(orders->isRelation);
}

void ClassParserTest::parsesPowershellClassProperties()
{
    const QString source = R"(
class Customer {
    [int]$Id
    [string]$Name
    [Order[]]$Orders
}
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Powershell);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Customer"));

    const ParsedAttribute *orders =
        findAttribute(classes.first(), "Orders");
    QVERIFY(orders != nullptr);
    QCOMPARE(orders->type, QString("Order[]"));
    QVERIFY(orders->isRelation);
}

void ClassParserTest::parsesJavaFields()
{
    const QString source = R"(
public class Customer {
    private int id;
    private String name;
    private List<Order> orders;
}
)";

    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            source,
            ProgrammingLanguage::ProgrammingLanguageType::Java);

    QCOMPARE(classes.size(), 1);
    QCOMPARE(classes.first().name, QString("Customer"));

    const ParsedAttribute *orders =
        findAttribute(classes.first(), "orders");
    QVERIFY(orders != nullptr);
    QCOMPARE(orders->type, QString("List<Order>"));
    QVERIFY(orders->isRelation);
}

void ClassParserTest::mapsLanguageTypesToSql()
{
    using Type = ProgrammingLanguage::ProgrammingLanguageType;

    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("String", Type::Java),
        QString("TEXT"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("LocalDateTime", Type::Java),
        QString("DATETIME"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("int", Type::Python),
        QString("INTEGER"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("time.Time", Type::Go),
        QString("DATETIME"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("[]byte", Type::Go),
        QString("BLOB"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("Option<String>", Type::Rust),
        QString("TEXT"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("DateTime", Type::Fsharp),
        QString("DATETIME"));
    QCOMPARE(
        ProgrammingLanguage::mapToSqlType("decimal", Type::Powershell),
        QString("NUMERIC"));

    QVERIFY(
        ProgrammingLanguage::isNullableType("int?"));
    QVERIFY(
        ProgrammingLanguage::isNullableType("string option"));
    QVERIFY(
        ProgrammingLanguage::isCollectionType("List<Order>"));
    QVERIFY(
        ProgrammingLanguage::isRelationshipType("Customer", Type::Python));
    QVERIFY(
        !ProgrammingLanguage::isRelationshipType("Vec<u8>", Type::Rust));
    QVERIFY(
        !ProgrammingLanguage::isRelationshipType("int", Type::Go));
}

void ClassParserTest::returnsEmptyListForContentWithoutClasses()
{
    ClassParser parser;
    const QList<ParsedClass> classes =
        parser.parseClasses(
            "int main() { return 0; }",
            ProgrammingLanguage::ProgrammingLanguageType::Cplusplus);

    QVERIFY(classes.isEmpty());
}

QTEST_MAIN(ClassParserTest)
#include "classparsertest.moc"
