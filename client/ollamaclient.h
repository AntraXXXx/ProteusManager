#ifndef OLLAMACLIENT_H
#define OLLAMACLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

class OllamaClient : public QObject
{
    Q_OBJECT

public:
    explicit OllamaClient(QObject * parent = nullptr);

    void checkConnection();
    void fetchModels();
    void generateSql(const QString& model,
                     const QString& prompt);
signals:
    void connectionChecked(bool isRunning);
    void modelsFetched(const QStringList& models);
    void responseReceived(const QString& response);
    void errorOccurred(const QString& errorMessage);
public:
    QString getLastResponse() const;
private:
    QNetworkAccessManager *m_networkManager;
    QString m_lastResponse;
};

#endif
