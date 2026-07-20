#include <QtTest>

#include "utils/normalizationplanner.h"

class NormalizationPlannerTest : public QObject
{
    Q_OBJECT

private slots:
    void ordersNormalForms();
    void choosesInputTablesForDirection();
    void insertsVersionsByNormalForm();
};

void NormalizationPlannerTest::ordersNormalForms()
{
    QCOMPARE(
        NormalizationPlanner::forms(),
        QStringList({"1NF", "2NF", "3NF", "BCNF", "4NF", "5NF"}));
    QCOMPARE(NormalizationPlanner::nextForm({}), QString("1NF"));
    QCOMPARE(NormalizationPlanner::nextForm("1NF"), QString("2NF"));
    QCOMPARE(NormalizationPlanner::nextForm("5NF"), QString());
    QCOMPARE(NormalizationPlanner::previousForm("5NF"), QString("4NF"));
    QCOMPARE(NormalizationPlanner::previousForm("1NF"), QString());
}

void NormalizationPlannerTest::choosesInputTablesForDirection()
{
    const QStringList original = {"Bestellungen"};
    const QStringList active = {
        "Bestellung_1NF", "Bestellposition_1NF"
    };

    QCOMPARE(
        NormalizationPlanner::inputTables(
            "2NF", "1NF", original, active),
        active);
    QCOMPARE(
        NormalizationPlanner::inputTables(
            "5NF", "1NF", original, active),
        active);
    QCOMPARE(
        NormalizationPlanner::inputTables(
            "1NF", "5NF", original, active),
        original);
    QCOMPARE(
        NormalizationPlanner::inputTables(
            "3NF", {}, original, {}),
        original);
}

void NormalizationPlannerTest::insertsVersionsByNormalForm()
{
    QCOMPARE(
        NormalizationPlanner::insertionIndex(
            "3NF", {QString(), "5NF"}),
        1);
    QCOMPARE(
        NormalizationPlanner::insertionIndex(
            "2NF", {QString(), "1NF", "3NF", "5NF"}),
        2);
    QCOMPARE(
        NormalizationPlanner::insertionIndex(
            "5NF", {QString(), "1NF", "BCNF"}),
        3);
}

QTEST_MAIN(NormalizationPlannerTest)
#include "normalizationplannertest.moc"
