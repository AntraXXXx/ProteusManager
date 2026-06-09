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
    enum class GenerateType
    {
        Sql,
        Dal
    };
    void generate(const QString& model, const QString& prompt, GenerateType type);
    void checkConnection();
    void fetchModels();
    bool isValidSqlCode();
signals:
    void connectionChecked(bool isRunning);
    void modelsFetched(const QStringList& models);
    void errorOccurred(const QString& errorMessage);
    void sqlReceived(const QString& sql);
    void dalReceived(const QString& code);
    void isSqlCode(bool isSqlCode);
public:
    QString getLastResponse() const;
    GenerateType generateType = GenerateType::Sql;
    bool isValidSql;
private:
    QNetworkAccessManager *m_networkManager;
    QString m_lastResponse;
};

#endif
