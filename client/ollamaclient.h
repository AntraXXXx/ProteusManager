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

signals:
    void connectionChecked(bool isRunning);
    void errorOccurred(const QString& errorMessage);

private:
    QNetworkAccessManager *m_networkManager;
};

#endif
