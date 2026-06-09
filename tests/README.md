# Testing ProteusManager

ProteusManager now has an initial automated test setup based on Qt Test and
CTest. The first tests focus on the stable core components that the QML UI calls
into.

## Run tests

From the project root:

```powershell
cmake --build build\Desktop_Qt_6_11_0_MinGW_64_bit-Debug
ctest --test-dir build\Desktop_Qt_6_11_0_MinGW_64_bit-Debug --output-on-failure
```

In Qt Creator, open the **Tests** view and run the CTest/Qt Test entries from
there. A `run_tests` build target is also available.

## Current coverage

- `ClassParserTest` verifies class and attribute parsing for C++ and Python.
- `ClassScannerTest` verifies language file filters and recursive class-file scanning.
- `DatabaseManagerTest` verifies SQLite connection handling, SQL validation,
  schema introspection, column checks, and row detection.
- `SqlValidationTest` verifies that generated SQL is limited to safe SQLite
  schema statements and rejects narrative or destructive AI output.
- `DalGenerationTest` verifies generated DAL file export and rejects unsafe file
  names from AI output.
- `SqlOutputRecorderTest` verifies that generated SQL is automatically written
  to a per-model `.sql` file.
- `OllamaClientTest` verifies model fetching, generated-response cleanup, and
  connection-error handling against a deterministic fake Ollama server.
- `OllamaEnvironmentTest` verifies endpoint validation, Ollama installation
  detection helpers, and setup guidance messages.
- `QmlWorkflowTest` verifies that the main QML pages load and react to
  generated SQL/DAL signals.

## Generated SQL output per model

Whenever an Ollama model returns SQL, `AppController` writes the raw SQL to:

```text
<AppDataLocation>/generated-sql/<model-name>_<timestamp>.sql
```

Example:

```text
llama3.2_latest_20260609_012345_678.sql
```

This makes it easier to compare outputs from different agents/models. The file
contains the executable SQL only, so it can be inspected or copied into a SQL
tool without removing report text.

## QML testing strategy

Most application logic should stay in C++ controller and service classes so it
can be tested with fast Qt Test unit tests. QML tests should be added as a
smaller smoke and workflow layer:

- Page-load smoke tests for `MainMenuPage`, `SqlGeneratorPage`, and
  `ProgrammingCodeGeneratorPage`.
- Workflow tests for database selection, SQL generation, SQL execution, DAL
  generation, and DAL export.
- Error-message tests for missing database, missing model, missing class folder,
  and invalid AI output.

The next practical step is to introduce a lightweight fake `AppController` for
QML tests. That keeps QML tests deterministic and avoids requiring Ollama or a
real database during UI test runs.
