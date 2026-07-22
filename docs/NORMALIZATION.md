# Database Normalization

ProteusManager keeps the original database tables and stores each applied normal form as a separate schema version. Selecting a target generates and validates a migration preview. The connected database changes only after the user chooses Apply Normalization.

## Flat Table Detection

ProteusManager profiles identifier-like columns and repeated sample values without writing to the source database. A flat table with no primary key and several embedded identifiers is treated as 1NF migration evidence because the target relation still needs a key that uniquely identifies every row.

For 2NF and higher, repeated determinants are compared with similarly named attributes. Evidence such as KundeID consistently determining KundeName and KundeEmail, or ProduktID consistently determining ProduktName, is included in the Ollama prompt and prevents an unsupported NO_CHANGES_REQUIRED result from being accepted. Identifier matching is case-insensitive, keeps the source language, and recognizes common identifier suffixes without translating generated names.

These findings are conservative migration evidence, not permission to discard values. Ollama must still generate a lossless CREATE TABLE plus INSERT INTO ... SELECT migration, and the existing preview validator must accept it before Apply Normalization becomes available.

Every created relation must declare a primary key or a suitable unique candidate key. For a flat source table without a declared key, the preview also verifies that all identifier-like columns remain represented in one connected target relationship graph and that every source column is read by the data-copy statements. Foreign-key columns must be populated explicitly.

When source rows exist, every created target table must contain copied rows in the isolated preview. A migration that drops a value, disconnects an identifier association, leaves a foreign key unpopulated, or creates an empty target is rejected. Ollama receives the exact validation findings and may regenerate the complete migration in up to four bounded repair attempts.

## SQLite Preview Safety

Generated SQLite migration SQL is never previewed on the live connection. ProteusManager creates a consistent temporary database with `VACUUM INTO`, opens a separate Qt SQL connection to that copy, executes the complete migration inside a transaction, checks foreign keys, and discards the copy.

If the copy cannot be created or opened, preview validation stops with an error. It does not fall back to executing generated DDL on the source database.

The migration validator continues to reject destructive statements such as `DROP`, `DELETE`, `TRUNCATE`, `UPDATE`, `REPLACE`, `ATTACH`, `DETACH`, and `VACUUM`. Applying a validated migration may create new normalized tables and copy existing values with `INSERT INTO ... SELECT`, but it must not remove the source tables or rows.

## Version Planning

Normal forms are ordered as `1NF`, `2NF`, `3NF`, `BCNF`, `4NF`, and `5NF`.

- Advancing to a higher form analyzes the active normalized tables.
- Generating a missing lower form starts from the original source tables.
- Stored versions are sorted by normal-form rank rather than creation time.
- Previous Level and Next Level select an existing adjacent version or generate the missing adjacent form.
- Activating an existing version changes application state only; it does not delete database tables.

## Entity-Relationship Models

Open Before ER Model refreshes only the original source tables from the connected database. Open After ER Model shows the currently selected migration preview, a pending stored version switch, or the active normalized version. A pending version always takes precedence when the view refreshes, so moving from NF1 through NF5 or back to an earlier saved level updates the open After model without executing SQL or deleting data.

Each entity lists all attributes and marks primary keys (PK), foreign keys (FK), unique attributes (UQ), and nullability. Relationship labels use Child.foreignKey -> Parent.referencedKey. The endpoints use Crow's Foot notation:

- 0..*: zero or many child rows
- 0..1: zero or one related row
- 1: exactly one required related row
- solid blue line: identifying relationship where the foreign key is part of the child primary key
- dashed gray line: non-identifying relationship

Cardinalities are derived from database constraints. SQLite uses table, foreign-key, and unique-index metadata. Migration previews derive the same metadata from PRIMARY KEY, FOREIGN KEY, REFERENCES, NOT NULL, and single-column UNIQUE constraints or indexes. When a driver cannot report uniqueness or nullability, ProteusManager displays the conservative optional/many relationship instead of claiming a stricter cardinality.

A connected database without tables produces an explicit status message instead of an inactive ER-model action.

## Validation

The automated regression scenarios create the reported German `Bestellungen` schema and the flat `Bestellungen_Flach` structure from `Test.db`, each with three rows. They verify dependency detection, key requirements, connected identifier relationships, and that successful and failing previews leave the source schema and rows unchanged. QML workflow coverage opens both diagram windows in a real `QQuickView` context.
