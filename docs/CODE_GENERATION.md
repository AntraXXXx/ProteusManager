# Code Generation

ProteusManager generates language-specific application code from the connected database schema. The generator uses deterministic language profiles around the Ollama request so the selected language, database provider, file extensions, layers, naming rules, and security requirements do not depend on the selected AI model.

## Available Layers

- Entity: persisted row data without database calls
- DTO: data transferred between layers without business or database logic
- Repository / DAO: the only layer allowed to contain SQL and driver calls
- Service / BLL: business workflows built on repository abstractions
- Controller / Presentation: input mapping and service coordination without SQL
- Domain Model: business behavior without database-driver dependencies

Repository implementations use the `Sql<TableName>` naming convention. The other generated types use the `Entity`, `Dto`, `Service`, `Controller`, and `Model` suffixes.

## Language-Aware Settings

The available settings change with the selected programming language. Object-oriented languages can expose Layered, Clean Architecture, and Hexagonal structures, while C offers Procedural and Layered output and PowerShell offers Script Module and Layered output. Unsupported layers and options are hidden instead of being sent to the model.

Database API choices are also language-specific. Examples include ADO.NET or Dapper for C#, DB-API or SQLAlchemy Core for Python, SQLx or Diesel for Rust, and JDBC or Spring JDBC for Java. Settings are stored separately for each language so switching languages does not reuse an incompatible API or architecture.

Every editable setting has a short `i` help button in the QML page. Page and section headings do not. Interfaces, asynchronous operations, and available layers use language-appropriate labels and defaults.

SQL injection protection is mandatory. The UI does not provide an option for disabling parameter binding.

## Simple And Advanced Modes

The default Secure Code view keeps the mandatory protections visible and groups the output into three scopes:

- Secure data access: entities and repository or DAO code
- Full application layers: data access, business, and presentation layers
- Custom configuration: the exact advanced layer selection

Architecture, database API, data-access pattern, interfaces, asynchronous operations, and individual layers remain available under `Show advanced settings`. They are collapsed by default so a user can generate a secure language profile without first understanding every architecture term.

## Language And Provider Profiles

| Language | SQLite | MySQL / MariaDB | PostgreSQL | SQL Server / ODBC |
| --- | --- | --- | --- | --- |
| C++ | Qt SQL | Qt SQL | Qt SQL | Qt SQL |
| C | SQLite C API | MySQL C API | libpq | ODBC C API |
| C# | Microsoft.Data.Sqlite | MySqlConnector | Npgsql | Microsoft.Data.SqlClient |
| Python | sqlite3 | mysql-connector-python | psycopg 3 | pyodbc |
| Go | modernc.org/sqlite | go-sql-driver/mysql | pgx stdlib | go-mssqldb |
| Rust | SQLx | SQLx | SQLx | odbc-api |
| F# | Microsoft.Data.Sqlite | MySqlConnector | Npgsql | Microsoft.Data.SqlClient |
| PowerShell | Microsoft.Data.Sqlite | MySqlConnector | Npgsql | Microsoft.Data.SqlClient |
| Java | SQLite JDBC | MySQL Connector/J | PostgreSQL JDBC | Microsoft JDBC Driver |

Every response also requests a `dependencies.txt` file containing the external packages required by the generated source.

## Project Assistant

The code workspace includes an optional Ollama assistant sidebar. Questions are sent with the selected language, database dialect, active schema, generation settings, and a bounded excerpt of the current generated code. The assistant is intended for project-specific architecture and implementation questions and keeps the same mandatory parameterized-SQL rules as generation.

Assistant conversations remain in application memory and can be cleared from the sidebar. They are not exported with generated source files.

## Validation And Repair

Before code can be exported, ProteusManager verifies:

- the response contains valid `FILE:` sections
- file names cannot escape the selected output directory
- file extensions match the selected language
- every requested layer exists for every table
- repository code contains language-specific parameter binding
- common dynamic SQL interpolation and concatenation patterns are absent
- requested test files are present

When the first AI response fails validation, ProteusManager sends the exact validation errors back to the selected model and requests one complete replacement file set. If the repaired response still fails, export remains disabled and the validation result is shown in the QML page.

These checks reduce model-dependent output differences, but they do not replace compiling the generated project with its real dependencies. Generated code must still be built and tested in the target application's CI environment before production use.

## Usage

1. Connect the target database.
2. Select the programming language on the main page.
3. Open Secure Code.
4. Select Secure data access or Full application layers. Open advanced settings only when the project needs a specific architecture, API, pattern, or layer selection.
5. Generate the secure file set.
6. Review the validation result and source code.
7. Export only after validation passes.
8. Open the optional Project Assistant for questions about the current project and generated output.
