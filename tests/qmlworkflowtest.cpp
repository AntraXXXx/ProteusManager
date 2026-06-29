#include <QtTest>

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

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
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint WRITE setOllamaEndpoint NOTIFY ollamaEndpointChanged)
    Q_PROPERTY(QString aiConnectionStatus READ aiConnectionStatus NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(QString aiSetupInstructions READ aiSetupInstructions NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(QStringList availableModels READ availableModels NOTIFY availableModelsChanged)
    Q_PROPERTY(bool ollamaInstalled READ ollamaInstalled NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(bool ollamaRunning READ ollamaRunning NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(bool aiEnvironmentReady READ aiEnvironmentReady NOTIFY aiEnvironmentChanged)
    Q_PROPERTY(QString normalizationOutput READ normalizationOutput NOTIFY normalizationChanged)
    Q_PROPERTY(QString normalizationStatus READ normalizationStatus NOTIFY normalizationChanged)
    Q_PROPERTY(QString selectedNormalizationForm READ selectedNormalizationForm NOTIFY normalizationChanged)
    Q_PROPERTY(QString appliedNormalizationForm READ appliedNormalizationForm NOTIFY normalizationChanged)
    Q_PROPERTY(bool normalizationReady READ normalizationReady NOTIFY normalizationChanged)
    Q_PROPERTY(bool canResetNormalization READ canResetNormalization NOTIFY normalizationChanged)
    Q_PROPERTY(bool canAdvanceNormalization READ canAdvanceNormalization NOTIFY normalizationChanged)
    Q_PROPERTY(QVariantList normalizationBeforeSchema READ normalizationBeforeSchema NOTIFY normalizationChanged)
    Q_PROPERTY(QVariantList normalizationAfterSchema READ normalizationAfterSchema NOTIFY normalizationChanged)

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
    QString ollamaEndpoint() const { return m_ollamaEndpoint; }
    QString aiConnectionStatus() const { return m_aiConnectionStatus; }
    QString aiSetupInstructions() const { return m_aiSetupInstructions; }
    QStringList availableModels() const { return m_availableModels; }
    bool ollamaInstalled() const { return m_ollamaInstalled; }
    bool ollamaRunning() const { return m_ollamaRunning; }
    bool aiEnvironmentReady() const { return m_aiEnvironmentReady; }
    QString normalizationOutput() const { return m_normalizationOutput; }
    QString normalizationStatus() const { return m_normalizationStatus; }
    QString selectedNormalizationForm() const { return m_selectedNormalizationForm; }
    QString appliedNormalizationForm() const { return m_appliedNormalizationForm; }
    bool normalizationReady() const { return m_normalizationReady; }
    bool canResetNormalization() const { return m_canResetNormalization; }
    bool canAdvanceNormalization() const { return m_canAdvanceNormalization; }
    QVariantList normalizationBeforeSchema() const { return m_normalizationBeforeSchema; }
    QVariantList normalizationAfterSchema() const { return m_normalizationAfterSchema; }

    Q_INVOKABLE QStringList codeLanguages() const
    {
        return {"C++", "Python"};
    }

    Q_INVOKABLE QStringList databaseDriverNames() const
    {
        return {
            "MySQL / MariaDB",
            "PostgreSQL",
            "SQL Server / ODBC"
        };
    }

    Q_INVOKABLE QStringList normalizationForms() const
    {
        return {"1NF", "2NF", "3NF", "BCNF", "4NF", "5NF"};
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

    Q_INVOKABLE void setOllamaEndpoint(const QString& endpoint)
    {
        m_ollamaEndpoint = endpoint;
        emit ollamaEndpointChanged();
    }

    Q_INVOKABLE void refreshAiEnvironment()
    {
        emit aiEnvironmentChanged();
        emit availableModelsChanged();
        emit modelsFetched(m_availableModels);
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

    Q_INVOKABLE void connectOnlineDatabase(
        const QString&,
        const QString&,
        const QString&,
        const QString&,
        const QString&,
        const QString&)
    {
        m_databaseConnected = true;
        emit databaseConnectedChanged(true);
    }

    Q_INVOKABLE void onGenerateNormalization(const QString& form)
    {
        m_selectedNormalizationForm = form;
        m_normalizationOutput =
            "CREATE TABLE CustomerAddress (customerId INTEGER, city TEXT);";
        m_normalizationStatus = "Migration preview is ready.";
        m_normalizationReady = true;
        m_normalizationAfterSchema = m_normalizationBeforeSchema;
        m_normalizationAfterSchema.append(QVariantMap{
            {"name", "CustomerAddress"},
            {"columns", QVariantList{}},
            {"relations", QVariantList{}},
            {"proposed", true}
        });
        emit normalizationChanged();
    }

    Q_INVOKABLE QString onApplyNormalization()
    {
        m_appliedNormalizationForm =
            m_selectedNormalizationForm;
        m_normalizationReady = false;
        m_normalizationStatus = "Normalization applied.";
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    Q_INVOKABLE QString onResetNormalization()
    {
        m_normalizationStatus = "Reset preview is ready.";
        m_normalizationReady = true;
        emit normalizationChanged();
        return m_normalizationStatus;
    }

    Q_INVOKABLE QString onAdvanceNormalization()
    {
        m_selectedNormalizationForm = "2NF";
        m_normalizationStatus = "Next version preview is ready.";
        m_normalizationReady = true;
        emit normalizationChanged();
        return m_normalizationStatus;
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
    void ollamaEndpointChanged();
    void aiEnvironmentChanged();
    void availableModelsChanged();
    void dalStatusChanged(const QString& status);
    void dalExportFinished(const QString& message);
    void normalizationChanged();

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
    QString m_ollamaEndpoint = "http://localhost:11434";
    QString m_aiConnectionStatus = "AI ready. Models installed: 1";
    QString m_aiSetupInstructions = "AI environment is ready.";
    QStringList m_availableModels = {"agent-one"};
    bool m_ollamaInstalled = true;
    bool m_ollamaRunning = true;
    bool m_aiEnvironmentReady = true;
    QString m_normalizationOutput;
    QString m_normalizationStatus = "No normalization has been applied.";
    QString m_selectedNormalizationForm;
    QString m_appliedNormalizationForm;
    bool m_normalizationReady = false;
    bool m_canResetNormalization = true;
    bool m_canAdvanceNormalization = true;
    QVariantList m_normalizationBeforeSchema = {
        QVariantMap{
            {"name", "Customer"},
            {"columns", QVariantList{}},
            {"relations", QVariantList{}},
            {"proposed", false}
        }
    };
    QVariantList m_normalizationAfterSchema =
        m_normalizationBeforeSchema;
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
    void loadsNormalizationPage();
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

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "AI Status:")
        != nullptr);
}

void QmlWorkflowTest::loadsNormalizationPage()
{
    QQmlEngine engine;
    FakeAppController controller;
    engine.rootContext()->setContextProperty(
        "appController",
        &controller);

    QQmlComponent component(
        &engine,
        QUrl::fromLocalFile(
            qmlFilePath("NormalizationPage.qml")));

    std::unique_ptr<QObject> page(component.create());

    QVERIFY2(
        page != nullptr,
        qPrintable(component.errorString()));

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Database Normalization")
        != nullptr);

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "5NF")
        != nullptr);

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Migration Preview")
        != nullptr);

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Open Before Diagram")
        != nullptr);

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Open After Diagram")
        != nullptr);

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Previous Level")
        != nullptr);

    QVERIFY(
        findObjectWithProperty(
            page.get(),
            "text",
            "Next Level")
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
