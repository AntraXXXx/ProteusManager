#include <QFile>
#include <QTextStream>
#include <QDebug>

void readTextFile(const QString& filePath)
{
    QFile file(filePath);

    // Datei öffnen
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Datei konnte nicht geöffnet werden!";
        return;
    }

    QTextStream in(&file);

    QString content = in.readAll();

    // Ausgabe
    qDebug() << "Dateiinhalt:";
    qDebug() << content;

    // Datei schließen
    file.close();
}

void readLineByLine(const QString& filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Fehler beim Öffnen!";
        return;
    }

    QTextStream in(&file);

    while (!in.atEnd())
    {
        QString line = in.readLine();

        qDebug() << line;
    }

    file.close();
}