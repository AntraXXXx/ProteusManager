#include "ollamaclient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

OllamaClient::OllamaClient(QObject *parent)
    : OllamaClient(QUrl("http://localhost:11434"), parent)
{
}

OllamaClient::OllamaClient(const QUrl& baseUrl, QObject *parent)
    : QObject(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_baseUrl(baseUrl)
{
}

QString OllamaClient::getLastResponse() const
{
    return m_lastResponse;
}

void OllamaClient::setBaseUrl(const QUrl& baseUrl)
{
    m_baseUrl = baseUrl;
}

QUrl OllamaClient::baseUrl() const
{
    return m_baseUrl;
}

QUrl OllamaClient::endpointUrl(const QString& path) const
{
    QUrl url = m_baseUrl;
    url.setPath(path);
    return url;
}

QString OllamaClient::cleanResponseText(QString response)
{
    response.remove(
        QRegularExpression(
            "<think>[\\s\\S]*?</think>",
            QRegularExpression::CaseInsensitiveOption));

    response.remove(
        QRegularExpression(
            "```[A-Za-z0-9_+#-]*\\s*"));

    response.remove("```");

    return response.trimmed();
}

void OllamaClient::checkConnection()
{
    QNetworkRequest request(endpointUrl("/api/tags"));
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        bool connected = reply->error() == QNetworkReply::NoError;

        if (connected)
        {
            qDebug() << "Ollama connection successful.";
        }
        else
        {
            qDebug() << "Ollama connection failed:";
            qDebug() << reply->errorString();

            emit errorOccurred(reply->errorString());
        }

        emit connectionChecked(connected);

        reply->deleteLater();
    });
}

void OllamaClient::fetchModels()
{
    QNetworkRequest request(endpointUrl("/api/tags"));
    QNetworkReply *reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QStringList models;

        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray modelArray = doc.object()["models"].toArray();

        for (const QJsonValue& value : modelArray) {
            QString name = value.toObject()["name"].toString();
            models.append(name);
        }

        emit modelsFetched(models);

        reply->deleteLater();
    });
}

void OllamaClient::generate(const QString& model,
                            const QString& prompt,
                            GenerateType type)
{
    QNetworkRequest request(endpointUrl("/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["model"] = model;
    body["prompt"] = prompt;
    body["stream"] = false;

    qDebug() << "Sending request to Ollama...";

    QNetworkReply *reply =
        m_networkManager->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, type]() {

        qDebug() << "Reply received";

        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "Network Error:";
            qDebug() << reply->errorString();

            emit errorOccurred(reply->errorString());

            reply->deleteLater();
            return;
        }

        QByteArray responseData = reply->readAll();

        QJsonDocument doc =
            QJsonDocument::fromJson(responseData);

        QString response =
            doc.object()["response"].toString();

        qDebug() << "AI Response:";
        qDebug() << response;

        response = cleanResponseText(response);

        m_lastResponse = "AI Response:\n" + response;

        if (type == GenerateType::Sql)
        {
            emit sqlReceived(response);
        }
        else if (type == GenerateType::Dal)
        {
            response = response.trimmed();

            emit dalReceived(response);
        }
        else if (type == GenerateType::Normalization)
        {
            emit normalizationReceived(
                response.trimmed());
        }

        reply->deleteLater();
    });
}
