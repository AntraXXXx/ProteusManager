# Database Normalization

ProteusManager keeps the original database tables and stores each applied normal form as a separate schema version. Selecting a target generates and validates a migration preview. The connected database changes only after the user chooses Apply Normalization.

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

## Schema Diagrams

Open Before Diagram refreshes the original schema directly from the connected database. Open After Diagram refreshes either the validated migration preview or the active normalized version. A connected database without tables produces an explicit status message instead of an inactive diagram action.

## Validation

The automated regression scenario creates the reported German `Bestellungen` schema with three rows. It verifies that successful and failing 1NF previews leave the original table, schema, and all three rows unchanged. QML workflow coverage opens both diagram windows in a real `QQuickView` context.
