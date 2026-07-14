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

## Architecture Settings

The generator supports Layered, Clean Architecture, and Hexagonal dependency directions. Repository and DAO can be selected as the data access pattern. Interfaces, asynchronous operations, and unit tests can be requested independently.

SQL injection protection is mandatory. The UI does not provide an option for disabling parameter binding.

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
3. Open the Code Generator.
4. Select the architecture, data access pattern, and required layers.
5. Generate the complete file set.
6. Review the validation result and source code.
7. Export only after validation passes.
