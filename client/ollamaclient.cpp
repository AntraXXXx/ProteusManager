#include "ollamaclient.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

OllamaClient::OllamaClient(QObject *parent)
    : QObject(parent),
    m_networkManager(new QNetworkAccessManager(this))
{
}

void OllamaClient::checkConnection()
{
    QNetworkRequest request(QUrl("http://localhost:11434/api/tags"));
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
    QNetworkRequest request(QUrl("http://localhost:11434/api/tags"));
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