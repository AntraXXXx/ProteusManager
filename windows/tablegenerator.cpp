#include "tablegenerator.h"
#include "ui_tablegenerator.h"
//#include "../utils/textfilemanager.h"
// #include "../utils/classscanner.h"
// #include "../utils/classparser.h"

Tablegenerator::Tablegenerator(DatabaseManager *databaseManager,
                               QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tablegenerator)
    , m_ollamaClient(new OllamaClient(this))
    , m_dataBaseManager(databaseManager)
{
    ui->setupUi(this);
    ui->progressBar_loading->hide();
    ui->pushButton_execute->setEnabled(false);

    connect(
        m_ollamaClient,
        &OllamaClient::sqlReceived,
        this,
        [this](const QString& response)
        {
           // ui->progressBar_loading->hide();
            ui->pushButton_generate->setEnabled(true);

            ui->plainTextEdit_ai->setPlainText(response);

            if(!response.isEmpty()){
            ui->pushButton_execute->setEnabled(true);
            }

        });

    connect(
        m_ollamaClient,
        &OllamaClient::dalReceived,
        this,
        [this](const QString& code)
        {
            ui->plainTextEdit_dal->setPlainText(code);
        });

    connect(
        m_ollamaClient,
        &OllamaClient::errorOccurred,
        this,
        [this](const QString& error)
        {
            ui->progressBar_loading->hide();
            ui->pushButton_generate->setEnabled(true);

            QMessageBox::critical(this, "Ollama Error", error);
        });
   // ui->pushButton_ConnectDB->setEnabled(true);
   // ui->pushButton_ConnectDB->setStyleSheet("background-color: none;");
  //  ui->pushButton_ConnectDB->setText("Connect");
   // ui->pushButton_SqlGenerator->setEnabled(false);

}

Tablegenerator::~Tablegenerator()
{
    delete ui;
}

void Tablegenerator::setSelectedModel(const QString& model)
{
    m_selectedModel = model;
}

void Tablegenerator::setSelectedLanguage(
    ProgrammingLanguage::ProgrammingLanguageType language)
{
    m_selectedLanguage = language;
}

void Tablegenerator::on_pushButton_addclasses_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select a folder");

    if (!folderPath.isEmpty()) {
        ui->lineEdit_classespath->setText(folderPath);
        m_classPath = folderPath;
    }
}

void Tablegenerator::on_pushButton_addoutputdal_clicked()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select a folder");

    if (!folderPath.isEmpty()) {
        ui->lineEdit_dalpath->setText(folderPath);
        m_dalPath = folderPath;
    }
}

void Tablegenerator::on_pushButton_generate_clicked()
{
    ui->progressBar_loading->setRange(0, 0);
    ui->progressBar_loading->show();
    ui->pushButton_generate->setEnabled(false);

    if (m_classPath.isEmpty())
    {
        QMessageBox::warning(this, "Missing path", "Please select a class folder first.");
        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    if (m_selectedModel.isEmpty())
    {
        QMessageBox::warning(this, "Missing AI model", "Please select an AI model first.");
        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    if (!m_dataBaseManager->isConnected())
    {
        QMessageBox::warning(
            this,
            "Database",
            "Please connect to a database first."
            );

        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    ClassScanner scanner;
    ClassParser parser;

    ui->plainTextEdit_ai->setPlainText(
        "Scanning class files...\n"
        );

    QList<ScannedClassFile> files =scanner.scanAndReadClassFiles(m_classPath, m_selectedLanguage);

    if (files.isEmpty())
    {
        QMessageBox::warning(
            this,
            "No files found",
            "No class files were found in the selected folder."
            );

        ui->progressBar_loading->hide();
        ui->pushButton_generate->setEnabled(true);
        return;
    }

    m_prompt =
        "You are a professional SQLite database architect. "
        "Generate SQL that is fully compatible with SQLite 3. "
        "Analyze all provided classes, attributes and database metadata. "
        "Never generate MySQL, PostgreSQL, SQL Server or Oracle syntax. "

        "Use only the provided class names, attribute names and database information. "
        "Never rename classes. "
        "Never rename attributes. "
        "Never replace an existing attribute with a different name. "
        "Attribute names must remain exactly as provided. "
        "If an attribute is named username, it must remain username. "
        "Do not convert username to name or any other synonym. "
        "Do not invent attributes that are not provided. "
        "Respect exact attribute names, spelling and casing. "

        "If a table does not exist, generate a CREATE TABLE statement. "
        "If a table already exists, do not recreate it. "
        "If an attribute is missing in an existing table, generate ALTER TABLE ADD COLUMN. "
        "If an attribute already exists, do not generate it again. "

        "Never drop, delete, recreate or overwrite existing tables. "
        "Never remove existing columns. "
        "Never destroy existing data. "
        "No Duplicate Columns: Never generate ALTER TABLE to add a new column if a field with the same logical purpose already exists (e.g., do not add name if Username is present)."
        "Semantic Mapping: Map fields by their meaning, not just exact names. If a matching concept exists under a slightly different name, reuse it instead of creating a new column."
        "Every detected class and attribute must be considered exactly as provided. "
        "Use appropriate SQLite datatypes. "
        "Create primary keys and foreign keys where appropriate. "
        "Use AUTOINCREMENT only when automatic identifier generation is logically required. ";

    if (ui->checkBox_aduitfields->isChecked())
    {
        m_prompt +=
            "Add createdAt and updatedAt columns to all generated tables. ";
    }
    else
    {
        m_prompt +=
            "Do not add createdAt or updatedAt unless they are explicitly defined in the provided classes. ";
    }

    m_prompt +=
        "Return only executable SQLite SQL statements. "
        "No markdown. "
        "No code fences. "
        "Do not wrap SQL in ```sql blocks. "
        "Return raw SQL only. "
        "No comments. "
        "No explanation.\n\n";

    ui->plainTextEdit_ai->setPlainText(
        "Parsing classes and attributes...\n"
        "Checking existing database tables...\n"
        "Checking existing columns...\n"
        );

    for (const ScannedClassFile& file : files)
    {
        QList<ParsedClass> classes =
            parser.parseClasses(
                file.content,
                m_selectedLanguage
                );

        for (const ParsedClass& cls : classes)
        {
            m_prompt += "Class: " + cls.name + "\n";

            if (m_dataBaseManager->tableExists(cls.name))
            {
                m_prompt += "Existing table: yes\n";

                if (m_dataBaseManager->hasRows(cls.name))
                    m_prompt += "Table contains data: yes\n";
                else
                    m_prompt += "Table contains data: no\n";

                QStringList existingColumns =
                    m_dataBaseManager->getColumnNames(cls.name);

                m_prompt += "Existing columns: "
                            + existingColumns.join(", ")
                            + "\n";
            }
            else
            {
                m_prompt += "Existing table: no\n";
            }

            for (const ParsedAttribute& attribute : cls.attributes)
            {
                m_prompt += "- "
                            + attribute.type
                            + " "
                            + attribute.name;

                if (m_dataBaseManager->tableExists(cls.name))
                {
                    if (m_dataBaseManager->columnExists(cls.name, attribute.name))
                        m_prompt += " existing_column";
                    else
                        m_prompt += " missing_column";
                }

                if (attribute.isRelation)
                    m_prompt += " relationship";

                m_prompt += "\n";
            }

            m_prompt += "\n";
        }
    }
    ui->plainTextEdit_ai->setPlainText(
        "Generating SQL schema with AI..."
        );
    //qDebug() << "Prompt:" << m_prompt;
    m_ollamaClient->generate(
        m_selectedModel,
        m_prompt,
        OllamaClient::GenerateType::Sql
        );
}

void Tablegenerator::on_pushButton_execute_clicked()
{
    QString response = ui->plainTextEdit_ai->toPlainText();

    if (!m_dataBaseManager->isValidSql(response))
    {
        QMessageBox::warning(
            this,
            "Invalid SQL",
            "The AI response does not contain valid SQL."
            );
        ui->progressBar_loading->hide();

        return;
    }

    if (m_dataBaseManager->executeQuery(response))
    {
        QMessageBox::information(
            this,
            "Database",
            "SQL executed successfully."
            );
        ui->progressBar_loading->hide();
        ui->pushButton_execute->setEnabled(false);
    }
}

void Tablegenerator::on_pushButton_back_clicked()
{
    this->close();
}

void Tablegenerator::closeEvent(QCloseEvent *event)
{
    emit windowClosed();

    QWidget::closeEvent(event);
}

void Tablegenerator::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    ui->plainTextEdit_dal->clear();
    ui->plainTextEdit_ai->clear();

    if (!m_dataBaseManager->isConnected())
    {
        ui->plainTextEdit_dal->setPlainText(
            "No database connection available."
            );

        ui->plainTextEdit_ai->setPlainText(
            "No database connection available."
            );
        return;
    }

    QString driver =
        m_dataBaseManager->databaseDriver();

    ui->plainTextEdit_dal->setPlainText(
        QString(
            "Connected database: %1\n\nReady to generate DAL."
            ).arg(driver)
        );

    ui->plainTextEdit_ai->setPlainText(
        QString(
            "Connected database: %1\n\nReady to generate SQL."
            ).arg(driver)
        );
}

void Tablegenerator::refreshUi()
{
    ui->pushButton_normalize->setHidden(true);

    ProgrammingLanguage programmingLanguage;
    ui->pushButton_generatedal->setText("Generate (DAL) " +  programmingLanguage.languageTypeToText(m_selectedLanguage));
    ui->pushButton_outputdal->setText("Output DAL " + programmingLanguage.languageTypeToText(m_selectedLanguage) + " Code");
    ui->checkBox_apiaccess->setText("Generate secure " + programmingLanguage.languageTypeToText(m_selectedLanguage) + " database access layer");
    ui->label_classfolderdir->setText(programmingLanguage.languageTypeToText(m_selectedLanguage) + "-Class-Folder-Direction");


    if (!m_dataBaseManager->isConnected())
    {
        ui->plainTextEdit_dal->setPlainText(
            "No database connection available."
            );

        ui->plainTextEdit_ai->setPlainText(
            "No database connection available."
            );

        return;
    }

    ui->plainTextEdit_dal->setPlainText(
        "Database connected and ready."
        );
}

void Tablegenerator::on_pushButton_normalize_clicked()
{
    ui->progressBar_loading->setRange(0, 0);
    ui->progressBar_loading->show();

    QString sqlPrompt =
        "You are a professional SQLite database normalization expert. "
        "Analyze the following SQLite schema. "
        "Normalize it up to 3NF where logically possible. "
        "Generate only safe executable SQL migration statements. "
        "Never drop data. Never delete tables. "
        "Do not rename existing tables. "
        "Do not assume JSON columns exist. "
        "Do not use json_extract unless the schema explicitly contains a JSON column. "
        "Only generate migration SQL based on the provided schema metadata. "
        "Prefer safe ALTER TABLE ADD COLUMN statements. "
        "Never generate destructive or risky migration steps without explicit confirmation. "
        "Return only SQL. No markdown. No explanation.\n\n";

    sqlPrompt += m_dataBaseManager->buildSchemaDescription();

    ui->plainTextEdit_ai->setPlainText(
        "Analyzing database schema..."
        );

    qDebug().noquote() << sqlPrompt;

    m_ollamaClient->generate(
        m_selectedModel,
        sqlPrompt,
        OllamaClient::GenerateType::Sql
        );
}

void Tablegenerator::on_pushButton_generatedal_clicked()
{
    if (!m_dataBaseManager->isConnected())
    {
        QMessageBox::warning(
            this,
            "DAL Generator",
            "No database connection available.");

        return;
    }

    QString dalPrompt = "You are a professional database access layer generator. ";

    if(ui->checkBox_apiaccess->isChecked()){
        dalPrompt +=  "Generate secure database access classes for the following SQLite schema. ";
        ui->plainTextEdit_dal->setPlainText(
            "Generating secure database access layer..."
            );
    }
    else{
         dalPrompt +=  "Generate database access classes for the following SQLite schema. ";
        ui->plainTextEdit_dal->setPlainText(
            "Generating database access layer..."
            );
    }

    dalPrompt += "Use real table names and real column names exactly as provided. "
        "Never use placeholder names like ExampleRepository. "
        "Never output placeholder tags like <header code> or <source code>. "
        "Use prepared statements or parameter binding for all values. "
        "Never concatenate user input into SQL strings. "
        "Generate classes with create, read, update and delete methods. ";

    ui->plainTextEdit_dal->setPlainText(
        "Analyzing language for DAL..."
        );

    if (m_selectedLanguage == ProgrammingLanguage::ProgrammingLanguageType::Cplusplus)
    {
        ui->plainTextEdit_dal->setPlainText(
            "Generating .h and .cpp files..."
            );

        dalPrompt +=
            "Target language: C++ with Qt. "
            "Use QSqlDatabase and QSqlQuery. "
            "Generate one .h file and one .cpp file per database table. ";
    }
    else if (m_selectedLanguage == ProgrammingLanguage::ProgrammingLanguageType::Csharp)
    {
        dalPrompt +=
            "Target language: C# with SQLite. "
            "Generate one .cs file per database table. "
            "Use parameterized queries. ";
    }

    dalPrompt +=
        "Return files exactly in this format:\n"
        "FILE: RealTableName.ext\n"
        "actual code\n\n"
        "No markdown. No explanation. No code fences.\n\n";

    dalPrompt += m_dataBaseManager->buildSchemaDescription();

    m_ollamaClient->generate(
        m_selectedModel,
        dalPrompt,
        OllamaClient::GenerateType::Dal
        );
}

void Tablegenerator::on_pushButton_outputdal_clicked()
{
    QString response = ui->plainTextEdit_dal->toPlainText();

    if (response.isEmpty())
        return;

    QStringList sections =
        response.split("FILE:", Qt::SkipEmptyParts);

    for (const QString& section : sections)
    {
        QString trimmed = section.trimmed();

        int lineEnd = trimmed.indexOf('\n');
        if (lineEnd == -1)
            continue;

        QString fileName = trimmed.left(lineEnd).trimmed();
        QString fileContent = trimmed.mid(lineEnd + 1);

        QFile file(m_dalPath + "/" + fileName);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << fileContent;
            file.close();
        }
    }

    QMessageBox::information(this, "DAL", "DAL files saved.");
}

