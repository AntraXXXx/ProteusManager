#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "utils/sqloutputrecorder.h"

class SqlOutputRecorderTest : public QObject
{
    Q_OBJECT

private slots:
    void recordsSqlOutputPerModel();
    void sanitizesEmptyModelName();
    void rejectsEmptySql();
};

void SqlOutputRecorderTest::recordsSqlOutputPerModel()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString sql =
        "CREATE TABLE Customer (id INTEGER PRIMARY KEY);";

    const SqlOutputRecord record =
        SqlOutputRecorder::recordGeneratedSql(
            "llama3.2:latest",
            sql,
            tempDir.path());

    QVERIFY(record.saved);
    QVERIFY(QFileInfo::exists(record.filePath));
    QVERIFY(QFileInfo(record.filePath).fileName()
                .startsWith("llama3.2_latest_"));
    QVERIFY(QFileInfo(record.filePath).fileName()
                .endsWith(".sql"));

    QFile outputFile(record.filePath);
    QVERIFY(outputFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(
        QString::fromUtf8(outputFile.readAll()).trimmed(),
        sql);
}

void SqlOutputRecorderTest::sanitizesEmptyModelName()
{
    QCOMPARE(
        SqlOutputRecorder::sanitizedModelName(""),
        QString("unknown-agent"));

    QCOMPARE(
        SqlOutputRecorder::sanitizedModelName("model/name:latest"),
        QString("model_name_latest"));
}

void SqlOutputRecorderTest::rejectsEmptySql()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const SqlOutputRecord record =
        SqlOutputRecorder::recordGeneratedSql(
            "agent",
            "   ",
            tempDir.path());

    QVERIFY(!record.saved);
    QCOMPARE(record.message, QString("No SQL output available."));
}

QTEST_MAIN(SqlOutputRecorderTest)
#include "sqloutputrecordertest.moc"
