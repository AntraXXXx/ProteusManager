#include <QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

#include "client/ollamaclient.h"

class FakeOllamaServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit FakeOllamaServer(
        bool failTagsEndpoint = false,
        QObject *parent = nullptr)
        : QTcpServer(parent)
        , m_failTagsEndpoint(failTagsEndpoint)
    {
        connect(
            this,
            &QTcpServer::newConnection,
            this,
            &FakeOllamaServer::handleNewConnection);
    }

    QUrl baseUrl() const
    {
        return QUrl(
            QString("http://127.0.0.1:%1")
                .arg(serverPort()));
    }

private slots:
    void handleNewConnection()
    {
        while (QTcpSocket *socket = nextPendingConnection())
        {
            connect(
                socket,
                &QTcpSocket::readyRead,
                this,
                [socket]()
                {
                    const QByteArray request =
                        socket->readAll();

                    QByteArray body;
                    int statusCode = 200;

                    FakeOllamaServer *server =
                        qobject_cast<FakeOllamaServer *>(
                            socket->parent());

                    const bool failTagsEndpoint =
                        server != nullptr
                        && server->m_failTagsEndpoint;

                    if (request.startsWith("GET /api/tags")
                        && failTagsEndpoint)
                    {
                        statusCode = 500;
                        body = "{\"error\":\"test failure\"}";
                    }
                    else if (request.startsWith("GET /api/tags"))
                    {
                        QJsonObject root;
                        root["models"] = QJsonArray{
                            QJsonObject{{"name", "agent-one"}},
                            QJsonObject{{"name", "agent-two"}}
                        };

                        body = QJsonDocument(root).toJson(
                            QJsonDocument::Compact);
                    }
                    else if (request.startsWith("POST /api/generate"))
                    {
                        QJsonObject root;
                        root["response"] =
                            "<think>internal reasoning</think>\n"
                            "```sql\n"
                            "CREATE TABLE Customer (id INTEGER PRIMARY KEY);\n"
                            "```";

                        body = QJsonDocument(root).toJson(
                            QJsonDocument::Compact);
                    }
                    else
                    {
                        statusCode = 404;
                        body = "{}";
                    }

                    const QByteArray header =
                        "HTTP/1.1 "
                        + QByteArray::number(statusCode)
                        + " OK\r\n"
                        + "Content-Type: application/json\r\n"
                        + "Content-Length: "
                        + QByteArray::number(body.size())
                        + "\r\n"
                        + "Connection: close\r\n\r\n";

                    socket->write(header + body);
                    socket->disconnectFromHost();
                });

            connect(
                socket,
                &QTcpSocket::disconnected,
                socket,
                &QObject::deleteLater);

            socket->setParent(this);
        }
    }

private:
    bool m_failTagsEndpoint = false;
};

class OllamaClientTest : public QObject
{
    Q_OBJECT

private slots:
    void fetchesModelsFromOllamaEndpoint();
    void cleansGeneratedSqlResponse();
    void routesNormalizationResponse();
    void reportsConnectionErrors();
};

void OllamaClientTest::fetchesModelsFromOllamaEndpoint()
{
    FakeOllamaServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    OllamaClient client(server.baseUrl());
    QSignalSpy modelsSpy(
        &client,
        &OllamaClient::modelsFetched);

    client.fetchModels();

    QVERIFY(modelsSpy.wait(2000));
    QCOMPARE(
        modelsSpy.takeFirst().at(0).toStringList(),
        QStringList({"agent-one", "agent-two"}));
}

void OllamaClientTest::cleansGeneratedSqlResponse()
{
    FakeOllamaServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    OllamaClient client(server.baseUrl());
    QSignalSpy sqlSpy(
        &client,
        &OllamaClient::sqlReceived);

    client.generate(
        "agent-one",
        "generate schema",
        OllamaClient::GenerateType::Sql);

    QVERIFY(sqlSpy.wait(2000));
    QCOMPARE(
        sqlSpy.takeFirst().at(0).toString(),
        QString("CREATE TABLE Customer (id INTEGER PRIMARY KEY);"));
}

void OllamaClientTest::routesNormalizationResponse()
{
    FakeOllamaServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));

    OllamaClient client(server.baseUrl());
    QSignalSpy normalizationSpy(
        &client,
        &OllamaClient::normalizationReceived);
    QSignalSpy sqlSpy(
        &client,
        &OllamaClient::sqlReceived);

    client.generate(
        "agent-one",
        "normalize schema",
        OllamaClient::GenerateType::Normalization);

    QVERIFY(normalizationSpy.wait(2000));
    QCOMPARE(normalizationSpy.count(), 1);
    QCOMPARE(sqlSpy.count(), 0);
}

void OllamaClientTest::reportsConnectionErrors()
{
    FakeOllamaServer server(true);
    QVERIFY(server.listen(QHostAddress::LocalHost));

    OllamaClient client(server.baseUrl());

    QSignalSpy errorSpy(
        &client,
        &OllamaClient::errorOccurred);

    client.fetchModels();

    QVERIFY(errorSpy.wait(2000));
    QVERIFY(!errorSpy.takeFirst().at(0).toString().isEmpty());
}

QTEST_MAIN(OllamaClientTest)
#include "ollamaclienttest.moc"
