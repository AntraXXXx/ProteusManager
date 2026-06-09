#include <QtTest>

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSet>

#include <memory>

class FakeAppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString selectedLanguageName READ selectedLanguageName NOTIFY languageChanged)
    Q_PROPERTY(bool executable READ executable NOTIFY executableChanged)
    Q_PROPERTY(bool databaseConnected READ databaseConnected NOTIFY databaseConnectedChanged)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(bool isLocalDatabase READ isLocalDatabase WRITE setIsLocalDatabase NOTIFY isLocalDatabaseChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString dalOutputPath READ dalOutputPath NOTIFY dalOutputPathChanged)
    Q_PROPERTY(QString classesFolderPath READ classesFolderPath NOTIFY classesFolderPathChanged)
    Q_PROPERTY(QString lastSqlOutputPath READ lastSqlOutputPath NOTIFY lastSqlOutputPathChanged)

public:
    QString selectedLanguageName() const { return m_languageName; }
    bool executable() const { return m_executable; }
    bool databaseConnected() const { return m_databaseConnected; }
    QString selectedModel() const { return m_selectedModel; }
    bool isLocalDatabase() const { return m_isLocalDatabase; }
    bool loading() const { return m_loading; }
    QString dalOutputPath() const { return m_dalOutputPath; }
    QString classesFolderPath() const { return m_classesFolderPath; }
    QString lastSqlOutputPath() const { return m_lastSqlOutputPath; }

    Q_INVOKABLE QStringList codeLanguages() const
    {
        return {"C++", "Python"};
    }

    Q_INVOKABLE void setSelectedLanguage(int index)
    {
        m_languageName = index == 1 ? "Python" : "C++";
        emit languageChanged();
    }

    Q_INVOKABLE void setSelectedModel(const QString& model)
    {
        m_selectedModel = model;
        emit selectedModelChanged();
    }

    Q_INVOKABLE void setDalOutputPath(const QString& path)
    {
        m_dalOutputPath = path;
        emit dalOutputPathChanged();
    }

    Q_INVOKABLE void setClassesFolderPath(const QString& path)
    {
        m_classesFolderPath = path;
        emit classesFolderPathChanged();
    }

    Q_INVOKABLE void setIsLocalDatabase(bool isLocal)
    {
        m_isLocalDatabase = isLocal;
        emit isLocalDatabaseChanged();
    }

    Q_INVOKABLE void setAddAuditFields(bool) {}

    Q_INVOKABLE void connectDatabase(const QString&)
    {
        m_databaseConnected = true;
        emit databaseConnectedChanged(true);
    }

    Q_INVOKABLE void onGenerateSqlCode()
    {
        emit sqlOutputChanged(
            "CREATE TABLE Customer (id INTEGER PRIMARY KEY);");
    }

    Q_INVOKABLE QString onExecuteSqlCode(const QString& response)
    {
        m_executable = false;
        emit executableChanged();
        return response;
    }

    Q_INVOKABLE void onGenerateDalCode(bool)
    {
        emit dalOutputChanged(
            "FILE: CustomerRepository.h\n"
            "class CustomerRepository {};");
    }

    Q_INVOKABLE void onExportDalCode(
        const QString&,
        const QString&)
    {
        emit dalExportFinished("DAL files saved: 1");
    }

    Q_INVOKABLE void fetchModels()
    {
        emit modelsFetched({"agent-one"});
    }

    void emitSqlOutput(const QString& sql)
    {
        emit sqlOutputChanged(sql);
    }

    void emitDalOutput(const QString& dal)
    {
        emit dalOutputChanged(dal);
    }

signals:
    void modelsFetched(const QStringList& models);
    void databaseConnectedChanged(bool connected);
    void databaseStatusChanged(const QString& status);
    void databasePathChanged(const QString& path);
    void selectedModelChanged();
    void isLocalDatabaseChanged();
    void languageChanged();
    void executableChanged();
    void loadingChanged();
    void sqlOutputChanged(const QString& text);
    void warningOccurred(const QString& title, const QString& message);
    void dalOutputChanged(const QString& path);
    void dalOutputPathChanged();
    void classesFolderPathChanged();
    void lastSqlOutputPathChanged();
    void sqlOutputSaved(const QString& path);
    void dalStatusChanged(const QString& status);
    void dalExportFinished(const QString& message);

private:
    QString m_languageName = "C++";
    bool m_executable = true;
    bool m_databaseConnected = false;
    QString m_selectedModel = "agent-one";
    bool m_isLocalDatabase = true;
    bool m_loading = false;
    QString m_dalOutputPath = "C:/temp";
    QString m_classesFolderPath = "C:/classes";
    QString m_lastSqlOutputPath;
};

namespace
{
QString qmlFilePath(const QString& fileName)
{
    return QString(PROTEUS_SOURCE_DIR)
           + "/qml/"
           + fileName;
}

QObject *findObjectWithProperty(
    QObject *root,
    const char *propertyName,
    const QVariant& value,
    QSet<QObject *>& visited)
{
    if (root == nullptr || visited.contains(root))
        return nullptr;

    visited.insert(root);

    if (root->property(propertyName) == value)
        return root;

    for (QObject *child : root->children())
    {
        if (QObject *match =
                findObjectWithProperty(
                    child,
                    propertyName,
                    value,
                    visited))
        {
            return match;
        }
    }

    if (QQuickItem *item = qobject_cast<QQuickItem *>(root))
    {
        for (QQuickItem *childItem : item->childItems())
        {
            if (QObject *match =
                    findObjectWithProperty(
                        childItem,
                        propertyName,
                        value,
                        visited))
            {
                return match;
            }
        }
    }

    return nullptr;
}

QObject *findObjectWithProperty(
    QObject *root,
    const char *propertyName,
    const QVariant& value)
{
    QSet<QObject *> visited;
    return findObjectWithProperty(
        root,
        propertyName,
        value,
        visited);
}
}

class QmlWorkflowTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsMainMenuPage();
    void sqlPageDisplaysGeneratedSql();
    void dalPageDisplaysGeneratedCode();
};

void QmlWorkflowTest::loadsMainMenuPage()
{
    QQmlEngine engine;
    FakeAppController controller;
    engine.rootContext()->setContextProperty(
        "appController",
        &controller);

    QQmlComponent component(
        &engine,
        QUrl::fromLocalFile(qmlFilePath("MainMenuPage.qml")));

    std::unique_ptr<QObject> page(component.create());

    QVERIFY2(
        page != nullptr,
        qPrintable(component.errorString()));

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Proteus Manager")
        != nullptr);
}

void QmlWorkflowTest::sqlPageDisplaysGeneratedSql()
{
    QQmlEngine engine;
    FakeAppController controller;
    engine.rootContext()->setContextProperty(
        "appController",
        &controller);

    QQmlComponent component(
        &engine,
        QUrl::fromLocalFile(qmlFilePath("SqlGeneratorPage.qml")));

    std::unique_ptr<QObject> page(component.create());

    QVERIFY2(
        page != nullptr,
        qPrintable(component.errorString()));

    QObject *outputArea =
        findObjectWithProperty(
            page.get(),
            "placeholderText",
            "Generated SQL will appear here...");

    QVERIFY(outputArea != nullptr);

    const QString sql =
        "CREATE TABLE Customer (id INTEGER PRIMARY KEY);";
    controller.emitSqlOutput(sql);
    QCoreApplication::processEvents();

    QCOMPARE(
        outputArea->property("text").toString(),
        sql);
}

void QmlWorkflowTest::dalPageDisplaysGeneratedCode()
{
    QQmlEngine engine;
    FakeAppController controller;
    engine.rootContext()->setContextProperty(
        "appController",
        &controller);

    QQmlComponent component(
        &engine,
        QUrl::fromLocalFile(
            qmlFilePath("ProgrammingCodeGeneratorPage.qml")));

    std::unique_ptr<QObject> page(component.create());

    QVERIFY2(
        page != nullptr,
        qPrintable(component.errorString()));

    QObject *outputArea =
        findObjectWithProperty(
            page.get(),
            "placeholderText",
            "Generated database access layer code will appear here...");

    QVERIFY(outputArea != nullptr);

    const QString dal =
        "FILE: CustomerRepository.h\n"
        "class CustomerRepository {};";
    controller.emitDalOutput(dal);
    QCoreApplication::processEvents();

    QCOMPARE(
        outputArea->property("text").toString(),
        dal);
}

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");

    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");

    QGuiApplication app(argc, argv);

    QmlWorkflowTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "qmlworkflowtest.moc"
