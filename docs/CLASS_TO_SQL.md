# Class To SQL

Class to SQL creates a database schema from source-code type declarations in the language selected on the main page.

## Accepted Input

The selected folder is scanned recursively with language-specific file filters. A source file contributes to generation only when the parser finds a supported class, record, or struct declaration with at least one named data field.

Object instances, method-only classes, empty classes, unrelated source files, and files with unsupported extensions do not become database tables. If no database-ready declaration is found, generation stops before an Ollama request is sent.

Supported declarations currently include C++ classes, C structs, C# classes, Python classes, Go structs, Rust structs, F# records, PowerShell classes, and Java classes.

## Workflow

1. Select the source and output language on the main page.
2. Connect the target database and open Class to SQL.
3. Select the folder containing source classes.
4. Optionally enable audit fields.
5. Create and review the SQL schema.
6. Apply the reviewed SQL to the connected database.

The connected database driver determines the SQL dialect. Existing table and column metadata is included in the generation request, and the prompt forbids destructive table replacement or removal of existing data.

## Verification

`ClassParserTest` verifies that object construction and method-only classes are excluded while database-ready declarations remain available. `QmlWorkflowTest` verifies the three-step workflow, contextual setting help, and compact-window action layout.
