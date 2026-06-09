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
    void parsesPythonConstructorAttributes();
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

void ClassParserTest::parsesPythonConstructorAttributes()
{
    const QString source = R"(
class Customer:
    def __init__(self):
        self.id: int = 0
        self.name: str = ""
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

    const ParsedAttribute *address =
        findAttribute(classes.first(), "address");
    QVERIFY(address != nullptr);
    QCOMPARE(address->type, QString("unknown"));
    QVERIFY(address->isRelation);
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
