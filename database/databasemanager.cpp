#include "databasemanager.h"

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlRecord>

#include <QDebug>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <QUuid>
#include <QVariantMap>

namespace
{
QString quoteSqliteIdentifier(const QString& identifier)
{
    QString quoted = identifier;
    quoted.replace("\"", "\"\"");
    return "\"" + quoted + "\"";
}

QString escapeTableIdentifier(const QSqlDatabase& db, const QString& tableName)
{
    const QString escaped =
        db.driver()
            ? db.driver()->escapeIdentifier(tableName, QSqlDriver::TableName)
            : QString();

    if (!escaped.isEmpty())
        return escaped;

    return quoteSqliteIdentifier(tableName);
}

QStringList sqlStatements(const QString& sql)
{
    return sql.split(
        ";",
        Qt::SkipEmptyParts);
}

QString stripLeadingSqlComments(QString statement)
{
    statement = statement.trimmed();

    while (statement.startsWith("--"))
    {
        const int lineEnd = statement.indexOf('\n');
        if (lineEnd == -1)
            return {};

        statement =
            statement.mid(lineEnd + 1).trimmed();
    }

    return statement;
}

QString cleanIdentifier(QString identifier)
{
    identifier = identifier.trimmed();

    if (identifier.size() >= 2)
    {
        const QChar first = identifier.front();
        const QChar last = identifier.back();

        if ((first == '"' && last == '"')
            || (first == '`' && last == '`')
            || (first == '[' && last == ']'))
        {
            identifier = identifier.mid(1, identifier.size() - 2);
        }
    }

    return identifier;
}

QStringList splitTableDefinitions(const QString& definitions)
{
    QStringList result;
    QString current;
    int depth = 0;
    QChar quote;

    for (const QChar character : definitions)
    {
        if (!quote.isNull())
        {
            current += character;
            if (character == quote)
                quote = {};
            continue;
        }

        if (character == '\'' || character == '"' || character == '`')
        {
            quote = character;
            current += character;
        }
        else if (character == '(')
        {
            ++depth;
            current += character;
        }
        else if (character == ')')
        {
            --depth;
            current += character;
        }
        else if (character == ',' && depth == 0)
        {
            result.append(current.trimmed());
            current.clear();
        }
        else
        {
            current += character;
        }
    }

    if (!current.trimmed().isEmpty())
        result.append(current.trimmed());

    return result;
}

QVariantMap relationEntry(
    const QString& column,
    const QString& referenceTable,
    const QString& referenceColumn)
{
    return {
        {"column", column},
        {"referenceTable", referenceTable},
        {"referenceColumn", referenceColumn}
    };
}

QVariantMap columnEntry(
    const QString& name,
    const QString& type,
    bool primaryKey,
    bool nullable,
    bool unique,
    const QVariantMap& relation = {})
{
    return {
        {"name", name},
        {"type", type},
        {"primaryKey", primaryKey},
        {"nullable", nullable},
        {"unique", unique},
        {"foreignKey", !relation.isEmpty()},
        {"referenceTable", relation.value("referenceTable")},
        {"referenceColumn", relation.value("referenceColumn")}
    };
}

void annotateRelationships(
    QVariantList& columns,
    QVariantList& relations)
{
    for (int relationIndex = 0;
         relationIndex < relations.size();
         ++relationIndex)
    {
        QVariantMap relation =
            relations.at(relationIndex).toMap();

        for (int columnIndex = 0;
             columnIndex < columns.size();
             ++columnIndex)
        {
            QVariantMap column =
                columns.at(columnIndex).toMap();
            if (column.value("name").toString().compare(
                    relation.value("column").toString(),
                    Qt::CaseInsensitive) != 0)
            {
                continue;
            }

            const bool identifying =
                column.value("primaryKey").toBool();
            const bool unique =
                column.value("unique").toBool();
            const bool nullable =
                column.value("nullable", true).toBool();

            relation["sourceCardinality"] =
                unique ? "0..1" : "0..*";
            relation["targetCardinality"] =
                nullable ? "0..1" : "1";
            relation["identifying"] = identifying;

            column["foreignKey"] = true;
            column["referenceTable"] =
                relation.value("referenceTable");
            column["referenceColumn"] =
                relation.value("referenceColumn");
            columns[columnIndex] = column;
            break;
        }

        relations[relationIndex] = relation;
    }
}

int tableIndex(const QVariantList& schema, const QString& tableName)
{
    for (int i = 0; i < schema.size(); ++i)
    {
        if (schema.at(i).toMap().value("name").toString().compare(
                tableName,
                Qt::CaseInsensitive) == 0)
        {
            return i;
        }
    }

    return -1;
}

QStringList repeatedColumnGroupDescriptions(
    const QVariantMap& table)
{
    const QRegularExpression numberedColumn(
        "^(.+?)[_\\-\\s]+(\\d+)[_\\-\\s]+(.+)$",
        QRegularExpression::CaseInsensitiveOption);
    QHash<QString, QSet<QString>> ordinalsByPrefix;
    QHash<QString, QSet<QString>> attributesByPrefix;
    QHash<QString, QString> displayPrefixByKey;

    for (const QVariant& columnValue :
         table.value("columns").toList())
    {
        const QString columnName =
            columnValue.toMap().value("name").toString();
        const QRegularExpressionMatch match =
            numberedColumn.match(columnName);

        if (!match.hasMatch())
            continue;

        const QString displayPrefix =
            cleanIdentifier(match.captured(1));
        const QString key =
            displayPrefix.toLower();
        displayPrefixByKey.insert(key, displayPrefix);
        ordinalsByPrefix[key].insert(match.captured(2));
        attributesByPrefix[key].insert(
            cleanIdentifier(match.captured(3)));
    }

    QStringList descriptions;
    for (auto iterator = ordinalsByPrefix.cbegin();
         iterator != ordinalsByPrefix.cend();
         ++iterator)
    {
        if (iterator.value().size() < 2)
            continue;

        QStringList ordinals =
            iterator.value().values();
        QStringList attributes =
            attributesByPrefix.value(iterator.key()).values();
        ordinals.sort(Qt::CaseInsensitive);
        attributes.sort(Qt::CaseInsensitive);

        descriptions.append(
            "prefix "
            + displayPrefixByKey.value(iterator.key())
            + ": ordinals="
            + ordinals.join(",")
            + ", attributes="
            + attributes.join(","));
    }

    descriptions.sort(Qt::CaseInsensitive);
    return descriptions;
}

bool identifierSignalsDenormalizedData(
    QString identifier)
{
    identifier = identifier.toLower();
    return identifier.contains("unnormalisiert")
           || identifier.contains("nicht_normalisiert")
           || identifier.contains("nichtnormalisiert")
           || identifier.contains("not_normalized")
           || identifier.contains("notnormalized")
           || identifier.contains("unnormalized")
           || identifier.contains("denormalized")
           || identifier.contains("denormalisiert");
}

bool columnNameSuggestsListValue(QString columnName)
{
    columnName = columnName.toLower();
    return columnName.contains("ids")
           || columnName.contains("codes")
           || columnName.contains("artikel")
           || columnName.contains("article")
           || columnName.contains("produkt")
           || columnName.contains("product")
           || columnName.contains("mengen")
           || columnName.contains("quantities")
           || columnName.contains("preise")
           || columnName.contains("prices")
           || columnName.contains("namen")
           || columnName.contains("names")
           || columnName.contains("liste")
           || columnName.contains("list");
}

QString compactIdentifier(const QString& identifier)
{
    QString compact;
    compact.reserve(identifier.size());

    for (const QChar character : identifier)
    {
        if (character.isLetterOrNumber())
            compact.append(character.toLower());
    }

    return compact;
}

QString identifierSuffix(const QString& identifier)
{
    const QString compact =
        compactIdentifier(identifier);
    const QStringList suffixes = {
        "identifier",
        "identifikator",
        "identifiant",
        "identificador",
        "nummer",
        "number",
        "codigo",
        "codice",
        "kennung",
        "code",
        "key",
        "id",
        "nr",
        "no"
    };

    for (const QString& suffix : suffixes)
    {
        if (compact.endsWith(suffix)
            && compact.size() > suffix.size() + 1)
        {
            return suffix;
        }
    }

    return {};
}

QString identifierStem(const QString& identifier)
{
    QString compact =
        compactIdentifier(identifier);
    const QString suffix =
        identifierSuffix(identifier);

    if (!suffix.isEmpty())
        compact.chop(suffix.size());

    return compact;
}

int commonPrefixLength(
    const QString& left,
    const QString& right)
{
    const int limit =
        qMin(left.size(), right.size());
    int length = 0;

    while (length < limit
           && left.at(length) == right.at(length))
    {
        ++length;
    }

    return length;
}

bool sharesEntityStem(
    const QString& identifierName,
    const QString& attributeName)
{
    const QString stem =
        identifierStem(identifierName);
    const QString attribute =
        compactIdentifier(attributeName);

    if (stem.size() < 3
        || attribute.size() < 3)
    {
        return false;
    }

    return attribute.startsWith(stem)
           || stem.startsWith(attribute)
           || commonPrefixLength(stem, attribute) >= 4;
}

QString profiledValueKey(const QVariant& value)
{
    if (!value.isValid() || value.isNull())
        return "<null>";

    return QString::number(value.metaType().id())
           + ":"
           + value.toString();
}

QStringList embeddedIdentifierNames(
    const QVariantMap& table)
{
    QStringList names;

    for (const QVariant& columnValue :
         table.value("columns").toList())
    {
        const QVariantMap column =
            columnValue.toMap();
        if (!column.value("foreignKey").toBool()
            && !identifierSuffix(
                    column.value("name")
                        .toString()).isEmpty())
        {
            names.append(
                column.value("name").toString());
        }
    }

    return names;
}

bool lacksDeclaredKeyWithEmbeddedIdentifiers(
    const QVariantMap& table)
{
    bool hasPrimaryKey = false;
    for (const QVariant& columnValue :
         table.value("columns").toList())
    {
        hasPrimaryKey =
            hasPrimaryKey
            || columnValue.toMap()
                   .value("primaryKey").toBool();
    }

    return !hasPrimaryKey
           && embeddedIdentifierNames(table).size() >= 2;
}

QStringList flatDependencyDescriptions(
    const QSqlDatabase& db,
    const QVariantMap& table,
    int profileLimit = 200)
{
    const QVariantList columns =
        table.value("columns").toList();
    QList<int> identifierColumns;
    QStringList identifierNames;

    for (int index = 0;
         index < columns.size();
         ++index)
    {
        const QVariantMap column =
            columns.at(index).toMap();
        const QString name =
            column.value("name").toString();
        if (column.value("foreignKey").toBool()
            || identifierSuffix(name).isEmpty())
        {
            continue;
        }

        identifierColumns.append(index);
        identifierNames.append(name);
    }

    QStringList descriptions;
    if (lacksDeclaredKeyWithEmbeddedIdentifiers(table)
        && identifierColumns.size() >= 2)
    {
        descriptions.append(
            "no declared primary key and multiple embedded identifier candidates: "
            + identifierNames.join(", "));
    }

    if (identifierColumns.isEmpty())
        return descriptions;

    const QString tableName =
        table.value("name").toString();
    const QString escapedTable =
        escapeTableIdentifier(db, tableName);
    const QString sampleSql =
        db.driverName().startsWith("QODBC")
            ? QString("SELECT TOP %1 * FROM %2")
                  .arg(profileLimit)
                  .arg(escapedTable)
            : QString("SELECT * FROM %1 LIMIT %2")
                  .arg(escapedTable)
                  .arg(profileLimit);
    QSqlQuery sampleQuery(db);
    if (!sampleQuery.exec(sampleSql))
        return descriptions;

    QList<QVariantList> rows;
    while (sampleQuery.next())
    {
        QVariantList row;
        row.reserve(columns.size());
        for (int index = 0;
             index < columns.size();
             ++index)
        {
            row.append(sampleQuery.value(index));
        }
        rows.append(row);
    }

    for (const int determinantIndex :
         identifierColumns)
    {
        const QString determinantName =
            columns.at(determinantIndex)
                .toMap()
                .value("name")
                .toString();
        QHash<QString, int> determinantCounts;

        for (const QVariantList& row : rows)
        {
            const QVariant determinant =
                row.value(determinantIndex);
            if (!determinant.isValid()
                || determinant.isNull())
            {
                continue;
            }

            const QString key =
                profiledValueKey(determinant);
            determinantCounts[key] =
                determinantCounts.value(key) + 1;
        }

        bool hasRepeatedValue = false;
        for (auto iterator =
                 determinantCounts.cbegin();
             iterator != determinantCounts.cend();
             ++iterator)
        {
            if (iterator.value() > 1)
            {
                hasRepeatedValue = true;
                break;
            }
        }

        if (!hasRepeatedValue
            || determinantCounts.size() < 2)
        {
            continue;
        }

        QStringList dependentAttributes;
        for (int dependentIndex = 0;
             dependentIndex < columns.size();
             ++dependentIndex)
        {
            if (dependentIndex == determinantIndex)
                continue;

            const QVariantMap dependentColumn =
                columns.at(dependentIndex).toMap();
            const QString dependentName =
                dependentColumn.value("name").toString();
            if (!identifierSuffix(dependentName).isEmpty()
                || !sharesEntityStem(
                    determinantName,
                    dependentName))
            {
                continue;
            }

            QHash<QString, QSet<QString>>
                valuesByDeterminant;
            QSet<QString> distinctValues;

            for (const QVariantList& row : rows)
            {
                const QVariant determinant =
                    row.value(determinantIndex);
                if (!determinant.isValid()
                    || determinant.isNull())
                {
                    continue;
                }

                const QString determinantKey =
                    profiledValueKey(determinant);
                const QString dependentValue =
                    profiledValueKey(
                        row.value(dependentIndex));
                valuesByDeterminant[determinantKey]
                    .insert(dependentValue);
                distinctValues.insert(dependentValue);
            }

            bool functionallyDependent =
                distinctValues.size() > 1;
            for (auto iterator =
                     valuesByDeterminant.cbegin();
                 iterator != valuesByDeterminant.cend();
                 ++iterator)
            {
                if (iterator.value().size() > 1)
                {
                    functionallyDependent = false;
                    break;
                }
            }

            if (functionallyDependent)
                dependentAttributes.append(dependentName);
        }

        if (!dependentAttributes.isEmpty())
        {
            descriptions.append(
                "repeated determinant "
                + determinantName
                + " consistently determines "
                + dependentAttributes.join(", ")
                + " in sampled rows");
        }
    }

    return descriptions;
}

QVariantList parseCreatedTables(const QString& migrationSql)
{
    QVariantList tables;
    const QString identifier =
        "[`\\\"\\[]?[A-Za-z_][A-Za-z0-9_]*[`\\\"\\]]?";
    const QRegularExpression createTable(
        "^CREATE\\s+TABLE\\s+(?:IF\\s+NOT\\s+EXISTS\\s+)?("
            + identifier
            + ")\\s*\\(([\\s\\S]*)\\)\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression columnDefinition(
        "^(" + identifier + ")\\s+([^\\s,]+)([\\s\\S]*)$",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression referenceDefinition(
        "\\bREFERENCES\\s+(" + identifier + ")\\s*\\(\\s*("
            + identifier
            + ")\\s*\\)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression tableForeignKey(
        "^(?:CONSTRAINT\\s+" + identifier + "\\s+)?FOREIGN\\s+KEY\\s*"
        "\\(\\s*(" + identifier + ")\\s*\\)\\s+REFERENCES\\s+("
            + identifier
            + ")\\s*\\(\\s*("
            + identifier
            + ")\\s*\\)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression tablePrimaryKey(
        "^(?:CONSTRAINT\\s+" + identifier + "\\s+)?PRIMARY\\s+KEY\\s*"
        "\\(([^)]*)\\)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression tableUnique(
        "^(?:CONSTRAINT\\s+" + identifier + "\\s+)?UNIQUE\\s*"
        "\\(([^)]*)\\)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression createUniqueIndex(
        "^CREATE\\s+UNIQUE\\s+INDEX\\s+(?:IF\\s+NOT\\s+EXISTS\\s+)?"
        + identifier + "\\s+ON\\s+(" + identifier
        + ")\\s*\\(([^)]*)\\)\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression inlinePrimaryKey(
        "\\bPRIMARY\\s+KEY\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression inlineNotNull(
        "\\bNOT\\s+NULL\\b",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression inlineUnique(
        "\\bUNIQUE\\b",
        QRegularExpression::CaseInsensitiveOption);

    for (QString statement : sqlStatements(migrationSql))
    {
        statement = stripLeadingSqlComments(statement);
        const QRegularExpressionMatch createMatch =
            createTable.match(statement.trimmed());

        if (!createMatch.hasMatch())
            continue;

        QVariantList columns;
        QVariantList relations;
        QSet<QString> tablePrimaryKeys;
        QSet<QString> tableUniqueColumns;
        bool hasPrimaryKey = false;
        bool hasUniqueKey = false;
        const QStringList definitions =
            splitTableDefinitions(createMatch.captured(2));

        for (const QString& definition : definitions)
        {
            const QRegularExpressionMatch primaryMatch =
                tablePrimaryKey.match(definition);
            if (primaryMatch.hasMatch())
            {
                hasPrimaryKey = true;
                for (const QString& key : primaryMatch.captured(1).split(','))
                    tablePrimaryKeys.insert(cleanIdentifier(key).toLower());
                continue;
            }

            const QRegularExpressionMatch uniqueMatch =
                tableUnique.match(definition);
            if (uniqueMatch.hasMatch())
            {
                hasUniqueKey = true;
                const QStringList keys =
                    uniqueMatch.captured(1).split(
                        ',',
                        Qt::SkipEmptyParts);
                if (keys.size() == 1)
                {
                    tableUniqueColumns.insert(
                        cleanIdentifier(keys.first()).toLower());
                }
                continue;
            }

            const QRegularExpressionMatch foreignMatch =
                tableForeignKey.match(definition);
            if (foreignMatch.hasMatch())
            {
                relations.append(relationEntry(
                    cleanIdentifier(foreignMatch.captured(1)),
                    cleanIdentifier(foreignMatch.captured(2)),
                    cleanIdentifier(foreignMatch.captured(3))));
                continue;
            }

            const QRegularExpressionMatch columnMatch =
                columnDefinition.match(definition);
            if (!columnMatch.hasMatch())
                continue;

            const QString name =
                cleanIdentifier(columnMatch.captured(1));
            const QString suffix =
                columnMatch.captured(3);
            const QRegularExpressionMatch referenceMatch =
                referenceDefinition.match(suffix);
            QVariantMap relation;

            if (referenceMatch.hasMatch())
            {
                relation = relationEntry(
                    name,
                    cleanIdentifier(referenceMatch.captured(1)),
                    cleanIdentifier(referenceMatch.captured(2)));
                relations.append(relation);
            }

            const bool primaryKey =
                inlinePrimaryKey.match(suffix).hasMatch();
            hasPrimaryKey = hasPrimaryKey || primaryKey;
            hasUniqueKey = hasUniqueKey
                           || inlineUnique.match(suffix).hasMatch();
            columns.append(columnEntry(
                name,
                columnMatch.captured(2),
                primaryKey,
                !primaryKey
                    && !inlineNotNull.match(suffix).hasMatch(),
                primaryKey
                    || inlineUnique.match(suffix).hasMatch(),
                relation));
        }

        for (int i = 0; i < columns.size(); ++i)
        {
            QVariantMap column = columns.at(i).toMap();
            const QString columnKey =
                column.value("name").toString().toLower();

            if (tablePrimaryKeys.contains(columnKey))
            {
                column["primaryKey"] = true;
                column["nullable"] = false;
                if (tablePrimaryKeys.size() == 1)
                    column["unique"] = true;
            }

            if (tableUniqueColumns.contains(columnKey))
                column["unique"] = true;

            columns[i] = column;
        }

        annotateRelationships(columns, relations);

        tables.append(QVariantMap{
            {"name", cleanIdentifier(createMatch.captured(1))},
            {"columns", columns},
            {"relations", relations},
            {"hasPrimaryKey", hasPrimaryKey},
            {"hasUniqueKey", hasUniqueKey},
            {"proposed", true}
        });
    }

    for (QString statement : sqlStatements(migrationSql))
    {
        statement = stripLeadingSqlComments(statement);
        const QRegularExpressionMatch indexMatch =
            createUniqueIndex.match(statement.trimmed());
        if (!indexMatch.hasMatch())
            continue;

        const QStringList indexedColumns =
            indexMatch.captured(2).split(
                ',',
                Qt::SkipEmptyParts);
        if (indexedColumns.size() != 1)
            continue;

        const int index = tableIndex(
            tables,
            cleanIdentifier(indexMatch.captured(1)));
        if (index < 0)
            continue;

        QVariantMap table = tables.at(index).toMap();
        table["hasUniqueKey"] = true;
        QVariantList columns =
            table.value("columns").toList();
        QVariantList relations =
            table.value("relations").toList();
        const QString uniqueColumn =
            cleanIdentifier(indexedColumns.first());

        for (int columnIndex = 0;
             columnIndex < columns.size();
             ++columnIndex)
        {
            QVariantMap column =
                columns.at(columnIndex).toMap();
            if (column.value("name").toString().compare(
                    uniqueColumn,
                    Qt::CaseInsensitive) == 0)
            {
                column["unique"] = true;
                columns[columnIndex] = column;
                break;
            }
        }

        annotateRelationships(columns, relations);
        table["columns"] = columns;
        table["relations"] = relations;
        tables[index] = table;
    }

    return tables;
}

QString firstUnkeyedCreatedTable(
    const QString& migrationSql)
{
    for (const QVariant& tableValue :
         parseCreatedTables(migrationSql))
    {
        const QVariantMap table = tableValue.toMap();
        if (!table.value("hasPrimaryKey").toBool()
            && !table.value("hasUniqueKey").toBool())
        {
            return table.value("name").toString();
        }
    }

    return {};
}

QString identifierAssociationError(
    const QVariantList& sourceSchema,
    const QVariantList& targetTables)
{
    QHash<QString, QSet<QString>> adjacency;
    QHash<QString, int> componentByTable;

    for (const QVariant& tableValue : targetTables)
    {
        const QString tableName =
            tableValue.toMap().value("name").toString();
        adjacency.insert(tableName.toLower(), {});
    }

    for (const QVariant& tableValue : targetTables)
    {
        const QVariantMap table = tableValue.toMap();
        const QString source =
            table.value("name").toString().toLower();
        for (const QVariant& relationValue :
             table.value("relations").toList())
        {
            const QString target =
                relationValue.toMap()
                    .value("referenceTable")
                    .toString()
                    .toLower();
            if (!adjacency.contains(target))
                continue;

            adjacency[source].insert(target);
            adjacency[target].insert(source);
        }
    }

    int component = 0;
    for (auto iterator = adjacency.cbegin();
         iterator != adjacency.cend();
         ++iterator)
    {
        if (componentByTable.contains(iterator.key()))
            continue;

        QStringList pending{iterator.key()};
        while (!pending.isEmpty())
        {
            const QString current = pending.takeLast();
            if (componentByTable.contains(current))
                continue;

            componentByTable.insert(current, component);
            for (const QString& related :
                 adjacency.value(current))
            {
                if (!componentByTable.contains(related))
                    pending.append(related);
            }
        }

        ++component;
    }

    for (const QVariant& sourceValue : sourceSchema)
    {
        const QVariantMap sourceTable =
            sourceValue.toMap();
        if (!lacksDeclaredKeyWithEmbeddedIdentifiers(
                sourceTable))
        {
            continue;
        }

        const QStringList identifiers =
            embeddedIdentifierNames(sourceTable);
        QSet<int> commonComponents;
        bool firstIdentifier = true;

        for (const QString& identifier : identifiers)
        {
            QSet<int> identifierComponents;
            for (const QVariant& targetValue : targetTables)
            {
                const QVariantMap targetTable =
                    targetValue.toMap();
                const QString targetName =
                    targetTable.value("name")
                        .toString().toLower();

                for (const QVariant& columnValue :
                     targetTable.value("columns").toList())
                {
                    if (compactIdentifier(
                            columnValue.toMap()
                                .value("name").toString())
                        == compactIdentifier(identifier))
                    {
                        identifierComponents.insert(
                            componentByTable.value(
                                targetName,
                                -1));
                    }
                }
            }

            identifierComponents.remove(-1);
            if (identifierComponents.isEmpty())
            {
                return "Migration does not preserve identifier '"
                       + identifier
                       + "' from source table '"
                       + sourceTable.value("name").toString()
                       + "'.";
            }

            if (firstIdentifier)
            {
                commonComponents = identifierComponents;
                firstIdentifier = false;
            }
            else
            {
                commonComponents.intersect(
                    identifierComponents);
            }
        }

        if (commonComponents.isEmpty())
        {
            return "Migration disconnects the source identifier association in table '"
                   + sourceTable.value("name").toString()
                   + "' between "
                   + identifiers.join(", ")
                   + ".";
        }
    }

    return {};
}

bool containsIdentifierToken(
    const QString& sql,
    const QString& identifier)
{
    const QString escaped =
        QRegularExpression::escape(
            cleanIdentifier(identifier));
    const QRegularExpression token(
        "(^|[^A-Za-z0-9_])[`\\\"\\[]?"
            + escaped
            + "[`\\\"\\]]?([^A-Za-z0-9_]|$)",
        QRegularExpression::CaseInsensitiveOption);
    return token.match(sql).hasMatch();
}

QStringList migrationCopyErrors(
    const QVariantList& sourceSchema,
    const QVariantList& targetTables,
    const QString& migrationSql)
{
    const QString identifier =
        "[`\\\"\\[]?[A-Za-z_][A-Za-z0-9_]*[`\\\"\\]]?";
    const QRegularExpression insertSelect(
        "\\bINSERT(?:\\s+OR\\s+IGNORE|\\s+IGNORE)?\\s+INTO\\s+("
            + identifier
            + ")\\s*(?:\\(([^)]*)\\))?[\\s\\S]*?\\bSELECT\\b([\\s\\S]*)$",
        QRegularExpression::CaseInsensitiveOption);
    QHash<QString, QSet<QString>> populatedColumns;
    QString copiedSourceExpressions;

    for (QString statement : sqlStatements(migrationSql))
    {
        statement = stripLeadingSqlComments(statement);
        const QRegularExpressionMatch match =
            insertSelect.match(statement.trimmed());
        if (!match.hasMatch())
            continue;

        const QString targetName =
            cleanIdentifier(match.captured(1));
        const int targetIndex =
            tableIndex(targetTables, targetName);
        if (targetIndex < 0)
            continue;

        const QString targetKey =
            targetName.toLower();
        const QString columnList =
            match.captured(2).trimmed();
        if (columnList.isEmpty())
        {
            for (const QVariant& columnValue :
                 targetTables.at(targetIndex)
                     .toMap()
                     .value("columns").toList())
            {
                populatedColumns[targetKey].insert(
                    columnValue.toMap()
                        .value("name").toString().toLower());
            }
        }
        else
        {
            for (const QString& column :
                 columnList.split(',', Qt::SkipEmptyParts))
            {
                populatedColumns[targetKey].insert(
                    cleanIdentifier(column).toLower());
            }
        }

        copiedSourceExpressions +=
            "\n" + match.captured(3);
    }

    QStringList errors;
    for (const QVariant& targetValue : targetTables)
    {
        const QVariantMap targetTable =
            targetValue.toMap();
        const QString targetName =
            targetTable.value("name").toString();
        const QString targetKey =
            targetName.toLower();
        if (!populatedColumns.contains(targetKey))
        {
            errors.append(
                "Created table '" + targetName
                + "' receives no INSERT INTO ... SELECT data copy.");
            continue;
        }

        for (const QVariant& relationValue :
             targetTable.value("relations").toList())
        {
            const QString foreignKey =
                relationValue.toMap()
                    .value("column").toString();
            if (!populatedColumns.value(targetKey).contains(
                    foreignKey.toLower()))
            {
                errors.append(
                    "Data copy into created table '"
                    + targetName
                    + "' does not populate foreign-key column '"
                    + foreignKey + "'.");
            }
        }
    }

    const bool selectsAllColumns =
        QRegularExpression(
            "(^|[^A-Za-z0-9_])\\*([^A-Za-z0-9_]|$)")
            .match(copiedSourceExpressions)
            .hasMatch();
    if (!selectsAllColumns)
    {
        for (const QVariant& sourceValue : sourceSchema)
        {
            const QVariantMap sourceTable =
                sourceValue.toMap();
            if (!lacksDeclaredKeyWithEmbeddedIdentifiers(
                    sourceTable))
            {
                continue;
            }

            for (const QVariant& columnValue :
                 sourceTable.value("columns").toList())
            {
                const QString columnName =
                    columnValue.toMap()
                        .value("name").toString();
                if (!containsIdentifierToken(
                        copiedSourceExpressions,
                        columnName))
                {
                    errors.append(
                        "Source column '" + columnName
                        + "' from table '"
                        + sourceTable.value("name").toString()
                        + "' is not read by any data-copy SELECT.");
                }
            }
        }
    }

    return errors;
}

bool schemaHasRows(
    const QSqlDatabase& db,
    const QVariantList& schema)
{
    for (const QVariant& tableValue : schema)
    {
        const QString tableName =
            tableValue.toMap().value("name").toString();
        QSqlQuery query(db);
        if (query.exec(
                "SELECT 1 FROM "
                + escapeTableIdentifier(db, tableName))
            && query.next())
        {
            return true;
        }
    }

    return false;
}

QString firstEmptyCreatedTable(
    const QSqlDatabase& db,
    const QVariantList& targetTables)
{
    for (const QVariant& tableValue : targetTables)
    {
        const QString tableName =
            tableValue.toMap().value("name").toString();
        QSqlQuery query(db);
        if (!query.exec(
                "SELECT 1 FROM "
                + escapeTableIdentifier(db, tableName))
            || !query.next())
        {
            return tableName;
        }
    }

    return {};
}

QString incompatibleMigrationFeature(
    const QString& driverName,
    const QString& sql)
{
    QString pattern;

    if (driverName == "QSQLITE")
    {
        pattern =
            "\\b(CHARINDEX|STRING_SPLIT|SPLIT_PART|UNNEST|GENERATE_SERIES|CONCAT)\\s*\\("
            "|\\bTOP\\s+\\d+\\b|\\bAUTO_INCREMENT\\b|\\bIDENTITY\\s*\\(";
    }
    else if (driverName.startsWith("QPSQL"))
    {
        pattern =
            "\\b(INSTR|IFNULL|CHARINDEX|STRING_SPLIT)\\s*\\("
            "|`|\\bAUTO_INCREMENT\\b|\\bIDENTITY\\s*\\(";
    }
    else if (driverName.startsWith("QMYSQL"))
    {
        pattern =
            "\\b(CHARINDEX|STRING_SPLIT|SPLIT_PART|UNNEST)\\s*\\("
            "|\\bTOP\\s+\\d+\\b|\\bIDENTITY\\s*\\(";
    }
    else if (driverName.startsWith("QODBC"))
    {
        pattern =
            "\\b(INSTR|SUBSTR|STRPOS|SPLIT_PART|UNNEST)\\s*\\("
            "|\\bLIMIT\\s+\\d+\\b|`|\\bAUTO_INCREMENT\\b|\\|\\|";
    }

    if (pattern.isEmpty())
        return {};

    const QRegularExpression incompatible(
        pattern,
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match =
        incompatible.match(sql);

    return match.hasMatch()
               ? match.captured(0)
               : QString();
}

}

bool DatabaseManager::openDatabase(const QString& connectionName,
                                   const QString& databasePath)
{
    if (connectionName.isEmpty() || databasePath.isEmpty())
    {
        m_lastError = "No local database path selected.";
        setConnection(false);
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::contains(connectionName)
            ? QSqlDatabase::database(connectionName)
            : QSqlDatabase::addDatabase(
                  "QSQLITE",
                  connectionName
                  );

    m_dataBaseConnectionName = connectionName;

    if (db.isOpen())
        db.close();

    db.setDatabaseName(databasePath);

    if (!db.open())
    {
        m_lastError = db.lastError().text();
        qDebug() << "Database Error:"
                 << m_lastError;

        setConnection(false);
        return false;
    }

    m_databasePath = databasePath;
    m_lastError.clear();
    qDebug() << "Database connected";

    setConnection(true);
    return true;
}

bool DatabaseManager::openRemoteDatabase(
    const QString& connectionName,
    const QString& driver,
    const QString& hostName,
    int port,
    const QString& databaseName,
    const QString& userName,
    const QString& password)
{
    if (connectionName.isEmpty()
        || driver.isEmpty()
        || hostName.isEmpty()
        || databaseName.isEmpty())
    {
        m_lastError =
            "Database type, database name and host name are required.";
        setConnection(false);
        return false;
    }

    if (!QSqlDatabase::drivers().contains(driver))
    {
        m_lastError =
            "Qt SQL driver is not installed: " + driver;
        qDebug() << m_lastError;
        setConnection(false);
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::contains(connectionName)
            ? QSqlDatabase::database(connectionName)
            : QSqlDatabase::addDatabase(
                  driver,
                  connectionName
                  );

    m_dataBaseConnectionName = connectionName;

    if (db.isOpen())
        db.close();

    db.setHostName(hostName);
    db.setDatabaseName(databaseName);
    db.setUserName(userName);
    db.setPassword(password);

    if (port > 0)
        db.setPort(port);

    if (!db.open())
    {
        m_lastError = db.lastError().text();
        qDebug() << "Remote database error:"
                 << m_lastError;

        setConnection(false);
        return false;
    }

    m_databasePath = databaseName;
    m_lastError.clear();
    qDebug() << "Remote database connected";

    setConnection(true);
    return true;
}

QString DatabaseManager::getSqlConnectionName() const
{
    return m_dataBaseConnectionName;
}

bool DatabaseManager::executeQuery(const QString& executeSqlCommand)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
    {
        m_lastError = "Database is not open.";
        qDebug() << "Database is not open";
        qDebug() << "Connection name:" << m_dataBaseConnectionName;
        return false;
    }

    QStringList queries =
        executeSqlCommand.split(";", Qt::SkipEmptyParts);

    for (QString queryString : queries)
    {
        queryString = queryString.trimmed();

        if (queryString.isEmpty())
            continue;

        QSqlQuery query(db);

        if (!query.exec(queryString))
        {
            m_lastError = query.lastError().text();
            qDebug() << "SQL Error:" << query.lastError().text();
            qDebug() << "Failed Query:" << queryString;
            m_isValidSql = false;
            return false;
        }

        qDebug() << "Executed:" << queryString;
    }
    m_isValidSql = true;
    return true;
}

void DatabaseManager::setDatabasePath(const QString& path)
{
    m_databasePath = path;
}

void DatabaseManager::setConnection(const bool isConnecting)
{
    m_isConnected = isConnecting;
}

bool DatabaseManager::isConnected() const
{
    return m_isConnected;
}

bool DatabaseManager::isLocalDatabase(bool isLocal)
{
    return isLocal;
}

bool DatabaseManager::tableExists(const QString& tableName)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
    {
        m_lastError = "Database is not open.";
        qDebug() << "Database is not open";
        return false;
    }

    const QStringList tables =
        db.tables(QSql::Tables);

    for (const QString& table : tables)
    {
        if (table.compare(tableName, Qt::CaseInsensitive) == 0)
            return true;
    }

    return false;
}

QStringList DatabaseManager::getTableNames()
{
    QSqlDatabase db = QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
        return {};

    QStringList tables =
        db.tables(QSql::Tables);

    tables.removeAll("sqlite_sequence");

    return tables;
}

QString DatabaseManager::buildSchemaDescription(
    const QStringList& tableNames)
{
    QString result =
        "Current "
        + databaseDriver()
        + " database schema:\n\n";

    const QStringList selectedTables =
        tableNames.isEmpty()
            ? getTableNames()
            : tableNames;

    for (const QString& table : selectedTables)
    {
        result += "Table: " + table + "\n";

        QSqlDatabase db =
            QSqlDatabase::database(m_dataBaseConnectionName);

        if (db.driverName() == "QSQLITE")
        {
            QSqlQuery query(
                QString("PRAGMA table_info(%1)")
                    .arg(quoteSqliteIdentifier(table)),
                db);

            while (query.next())
            {
                result += "- "
                          + query.value("name").toString()
                          + " "
                          + query.value("type").toString()
                          + "\n";
            }
        }
        else
        {
            const QSqlRecord record =
                db.record(table);

            for (int i = 0; i < record.count(); ++i)
            {
                const QSqlField field =
                    record.field(i);

                result += "- "
                          + field.name()
                          + " "
                          + QString::fromUtf8(field.metaType().name())
                          + "\n";
            }
        }

        result += "\n";
    }

    return result;
}

QString DatabaseManager::buildNormalizationAnalysis(
    const QStringList& tableNames,
    int sampleLimit)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);
    if (!db.isOpen())
        return {};

    sampleLimit = qBound(1, sampleLimit, 10);
    QString result =
        "Read-only normalization analysis. Identifier names are authoritative; "
        "infer their natural language and keep new names in the same language.\n"
        "Delimiter findings are evidence only. A comma in an address is not "
        "automatically a repeating group. Compare related columns and token counts.\n"
        "Numbered columns such as product_1_name, product_2_name, produkt_1_name "
        "or produkt_2_name are repeating column group evidence.\n\n";
    const QVariantList schema =
        tableNames.isEmpty()
            ? buildSchemaDiagram()
            : buildSchemaDiagramForTables(tableNames);

    const auto printableValue = [](const QVariant& value)
    {
        if (!value.isValid() || value.isNull())
            return QString("NULL");

        if (value.metaType().id() == QMetaType::QByteArray)
            return QString("<binary>");

        QString text = value.toString();
        text.replace('\\', "\\\\");
        text.replace('\n', "\\n");
        text.replace('\r', "\\r");
        text.replace('\'', "''");
        if (text.size() > 120)
            text = text.left(117) + "...";
        return "'" + text + "'";
    };

    for (const QVariant& tableValue : schema)
    {
        const QVariantMap table = tableValue.toMap();
        const QString tableName =
            table.value("name").toString();
        const QString escapedTable =
            escapeTableIdentifier(db, tableName);
        result += "Table: " + tableName + "\nColumns:\n";

        for (const QVariant& columnValue :
             table.value("columns").toList())
        {
            const QVariantMap column = columnValue.toMap();
            result += "- "
                      + column.value("name").toString()
                      + " "
                      + column.value("type").toString();
            if (column.value("primaryKey").toBool())
                result += " [PK]";
            if (column.value("foreignKey").toBool())
            {
                result += " [FK -> "
                          + column.value("referenceTable").toString()
                          + "."
                          + column.value("referenceColumn").toString()
                          + "]";
            }
            result += "\n";
        }

        const QStringList repeatedGroups =
            repeatedColumnGroupDescriptions(table);
        if (!repeatedGroups.isEmpty())
        {
            result += "Repeated numbered column group evidence:\n";
            for (const QString& group : repeatedGroups)
                result += "- " + group + "\n";
        }

        const QStringList dependencyEvidence =
            flatDependencyDescriptions(db, table);
        if (!dependencyEvidence.isEmpty())
        {
            result += "Functional dependency evidence:\n";
            for (const QString& dependency :
                 dependencyEvidence)
            {
                result += "- " + dependency + "\n";
            }
        }

        QSqlQuery countQuery(db);
        if (countQuery.exec(
                "SELECT COUNT(*) FROM " + escapedTable)
            && countQuery.next())
        {
            result += "Row count: "
                      + countQuery.value(0).toString()
                      + "\n";
        }

        constexpr int profileLimit = 200;
        const QString sampleSql =
            db.driverName().startsWith("QODBC")
                ? QString("SELECT TOP %1 * FROM %2")
                      .arg(profileLimit)
                      .arg(escapedTable)
                : QString("SELECT * FROM %1 LIMIT %2")
                      .arg(escapedTable)
                      .arg(profileLimit);
        QSqlQuery sampleQuery(db);
        QHash<QString, QStringList> delimiterProfiles;
        int sampleNumber = 0;

        if (sampleQuery.exec(sampleSql))
        {
            const QSqlRecord record = sampleQuery.record();
            result += "Sample rows (maximum "
                      + QString::number(sampleLimit)
                      + "):\n";

            while (sampleQuery.next())
            {
                ++sampleNumber;
                const bool includeSample =
                    sampleNumber <= sampleLimit;
                if (includeSample)
                {
                    result += "- row "
                              + QString::number(sampleNumber)
                              + ": ";
                }

                for (int i = 0; i < record.count(); ++i)
                {
                    const QString columnName =
                        record.fieldName(i);
                    const QVariant value = sampleQuery.value(i);
                    if (includeSample)
                    {
                        if (i > 0)
                            result += " | ";
                        result += columnName
                                  + "="
                                  + printableValue(value);
                    }

                    const QString text = value.toString();
                    const QStringList delimiters = {",", ";", "|"};
                    for (const QString& delimiter : delimiters)
                    {
                        const int tokenCount =
                            text.split(
                                    delimiter,
                                    Qt::SkipEmptyParts)
                                .size();
                        if (tokenCount > 1)
                        {
                            const QString key =
                                columnName + "\t" + delimiter;
                            delimiterProfiles[key].append(
                                QString::number(tokenCount));
                        }
                    }
                }

                if (includeSample)
                    result += "\n";
            }

            result += "Profiled rows for delimiter evidence: "
                      + QString::number(sampleNumber)
                      + " (maximum "
                      + QString::number(profileLimit)
                      + ")\n";
        }

        if (!delimiterProfiles.isEmpty())
        {
            result += "Possible delimited-value evidence:\n";
            for (auto iterator = delimiterProfiles.cbegin();
                 iterator != delimiterProfiles.cend();
                 ++iterator)
            {
                const QStringList keyParts =
                    iterator.key().split('\t');
                const QString delimiterName =
                    keyParts.value(1) == ","
                        ? "comma"
                        : keyParts.value(1) == ";"
                              ? "semicolon"
                              : "pipe";
                result += "- column "
                          + keyParts.value(0)
                          + ": delimiter="
                          + delimiterName
                          + ", token-counts="
                          + iterator.value().join(",")
                          + "\n";
            }
        }

        result += "\n";
    }

    return result;
}

bool DatabaseManager::hasNormalizationEvidence(
    const QStringList& tableNames,
    const QString& targetForm)
{
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);
    if (!db.isOpen())
        return false;

    const QVariantList schema =
        tableNames.isEmpty()
            ? buildSchemaDiagram()
            : buildSchemaDiagramForTables(tableNames);

    for (const QVariant& tableValue : schema)
    {
        const QVariantMap table = tableValue.toMap();
        const QString tableName =
            table.value("name").toString();

        if (identifierSignalsDenormalizedData(tableName))
            return true;

        if (!repeatedColumnGroupDescriptions(table).isEmpty())
            return true;

        if (lacksDeclaredKeyWithEmbeddedIdentifiers(table))
            return true;
        const bool requiresDependencyNormalization =
            targetForm.isEmpty()
            || targetForm.compare(
                   "1NF",
                   Qt::CaseInsensitive) != 0;
        if (requiresDependencyNormalization
            && !flatDependencyDescriptions(
                    db,
                    table).isEmpty())
        {
            return true;
        }
        const QString escapedTable =
            escapeTableIdentifier(db, tableName);
        const QString sampleSql =
            db.driverName().startsWith("QODBC")
                ? QString("SELECT TOP 200 * FROM %1")
                      .arg(escapedTable)
                : QString("SELECT * FROM %1 LIMIT 200")
                      .arg(escapedTable);
        QSqlQuery sampleQuery(db);

        if (!sampleQuery.exec(sampleSql))
            continue;

        const QSqlRecord record = sampleQuery.record();
        const QStringList delimiters = {",", ";", "|"};
        while (sampleQuery.next())
        {
            QHash<QString, int> alignedTokenCounts;

            for (int i = 0; i < record.count(); ++i)
            {
                const QString columnName =
                    record.fieldName(i);
                const QString text =
                    sampleQuery.value(i).toString();

                for (const QString& delimiter : delimiters)
                {
                    const int tokenCount =
                        text.split(
                                delimiter,
                                Qt::SkipEmptyParts)
                            .size();
                    if (tokenCount <= 1)
                        continue;

                    if (columnNameSuggestsListValue(columnName))
                        return true;

                    const QString key =
                        delimiter
                        + ":"
                        + QString::number(tokenCount);
                    alignedTokenCounts[key] =
                        alignedTokenCounts.value(key) + 1;
                    if (alignedTokenCounts.value(key) >= 2)
                        return true;
                }
            }
        }
    }

    return false;
}

QString DatabaseManager::databaseDriver() const
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isValid())
        return "database";

    return db.driverName();
}

bool DatabaseManager::columnExists(
    const QString& tableName,
    const QString& columnName)
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isOpen())
        return false;

    const QSqlRecord record =
        db.record(tableName);

    for (int i = 0; i < record.count(); ++i)
    {
        if (record.fieldName(i).compare(
                columnName,
                Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }

    return false;
}

bool DatabaseManager::hasRows(
    const QString& tableName)
{
    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    QSqlQuery query(db);

    query.exec(
        QString(
            "SELECT COUNT(*) FROM %1")
            .arg(escapeTableIdentifier(db, tableName)));

    if (query.next())
    {
        return query.value(0).toInt() > 0;
    }

    return false;
}

QStringList DatabaseManager::getColumnNames(
    const QString& tableName)
{
    QStringList columns;

    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isOpen())
        return columns;

    const QSqlRecord record =
        db.record(tableName);

    for (int i = 0; i < record.count(); ++i)
    {
        columns.append(
            record.fieldName(i)
            );
    }

    return columns;
}

QVariantList DatabaseManager::buildSchemaDiagram()
{
    QVariantList schema;
    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);

    if (!db.isOpen())
        return schema;

    QStringList tables = getTableNames();
    tables.sort(Qt::CaseInsensitive);

    for (const QString& table : tables)
    {
        QVariantList columns;
        QVariantList relations;
        QHash<QString, QVariantMap> relationByColumn;

        if (db.driverName() == "QSQLITE")
        {
            QSqlQuery foreignKeys(
                QString("PRAGMA foreign_key_list(%1)")
                    .arg(quoteSqliteIdentifier(table)),
                db);

            while (foreignKeys.next())
            {
                const QVariantMap relation = relationEntry(
                    foreignKeys.value(3).toString(),
                    foreignKeys.value(2).toString(),
                    foreignKeys.value(4).toString());
                relations.append(relation);
                relationByColumn.insert(
                    foreignKeys.value(3).toString().toLower(),
                    relation);
            }

            QSet<QString> uniqueColumns;
            QSqlQuery indexList(
                QString("PRAGMA index_list(%1)")
                    .arg(quoteSqliteIdentifier(table)),
                db);
            while (indexList.next())
            {
                if (!indexList.value(2).toBool())
                    continue;

                const QString indexName =
                    indexList.value(1).toString();
                QSqlQuery indexInfo(
                    QString("PRAGMA index_info(%1)")
                        .arg(quoteSqliteIdentifier(indexName)),
                    db);
                QStringList indexedColumns;
                while (indexInfo.next())
                {
                    const QString columnName =
                        indexInfo.value(2).toString();
                    if (!columnName.isEmpty())
                        indexedColumns.append(columnName);
                }

                if (indexedColumns.size() == 1)
                {
                    uniqueColumns.insert(
                        indexedColumns.first().toLower());
                }
            }

            int primaryKeyCount = 0;
            QSqlQuery tableInfo(
                QString("PRAGMA table_info(%1)")
                    .arg(quoteSqliteIdentifier(table)),
                db);

            while (tableInfo.next())
            {
                const QString name =
                    tableInfo.value(1).toString();
                const bool primaryKey =
                    tableInfo.value(5).toInt() > 0;
                if (primaryKey)
                    ++primaryKeyCount;

                columns.append(columnEntry(
                    name,
                    tableInfo.value(2).toString(),
                    primaryKey,
                    !primaryKey
                        && tableInfo.value(3).toInt() == 0,
                    uniqueColumns.contains(name.toLower()),
                    relationByColumn.value(name.toLower())));
            }

            if (primaryKeyCount == 1)
            {
                for (int columnIndex = 0;
                     columnIndex < columns.size();
                     ++columnIndex)
                {
                    QVariantMap column =
                        columns.at(columnIndex).toMap();
                    if (!column.value("primaryKey").toBool())
                        continue;

                    column["unique"] = true;
                    columns[columnIndex] = column;
                    break;
                }
            }
        }
        else
        {
            QSqlQuery foreignKeys(db);

            if (db.driverName().startsWith("QMYSQL"))
            {
                foreignKeys.prepare(
                    "SELECT COLUMN_NAME, REFERENCED_TABLE_NAME, "
                    "REFERENCED_COLUMN_NAME "
                    "FROM information_schema.KEY_COLUMN_USAGE "
                    "WHERE TABLE_SCHEMA = DATABASE() "
                    "AND TABLE_NAME = ? "
                    "AND REFERENCED_TABLE_NAME IS NOT NULL");
                foreignKeys.addBindValue(table);
                foreignKeys.exec();
            }
            else if (db.driverName().startsWith("QPSQL"))
            {
                foreignKeys.prepare(
                    "SELECT kcu.column_name, ccu.table_name, "
                    "ccu.column_name "
                    "FROM information_schema.table_constraints tc "
                    "JOIN information_schema.key_column_usage kcu "
                    "ON tc.constraint_name = kcu.constraint_name "
                    "AND tc.table_schema = kcu.table_schema "
                    "JOIN information_schema.constraint_column_usage ccu "
                    "ON ccu.constraint_name = tc.constraint_name "
                    "AND ccu.table_schema = tc.table_schema "
                    "WHERE tc.constraint_type = 'FOREIGN KEY' "
                    "AND tc.table_schema = current_schema() "
                    "AND tc.table_name = ?");
                foreignKeys.addBindValue(table);
                foreignKeys.exec();
            }
            else if (db.driverName().startsWith("QODBC"))
            {
                foreignKeys.prepare(
                    "SELECT fk.COLUMN_NAME, pk.TABLE_NAME, "
                    "pk.COLUMN_NAME "
                    "FROM INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS rc "
                    "JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE fk "
                    "ON fk.CONSTRAINT_NAME = rc.CONSTRAINT_NAME "
                    "JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE pk "
                    "ON pk.CONSTRAINT_NAME = rc.UNIQUE_CONSTRAINT_NAME "
                    "AND pk.ORDINAL_POSITION = fk.ORDINAL_POSITION "
                    "WHERE fk.TABLE_NAME = ?");
                foreignKeys.addBindValue(table);
                foreignKeys.exec();
            }

            while (foreignKeys.next())
            {
                const QVariantMap relation = relationEntry(
                    foreignKeys.value(0).toString(),
                    foreignKeys.value(1).toString(),
                    foreignKeys.value(2).toString());
                relations.append(relation);
                relationByColumn.insert(
                    foreignKeys.value(0).toString().toLower(),
                    relation);
            }

            const QSqlRecord record = db.record(table);
            const QSqlIndex primaryKey = db.primaryIndex(table);
            const bool singleColumnPrimaryKey =
                primaryKey.count() == 1;

            for (int i = 0; i < record.count(); ++i)
            {
                const QSqlField field = record.field(i);
                const bool isPrimaryKey =
                    primaryKey.indexOf(field.name()) >= 0;
                columns.append(columnEntry(
                    field.name(),
                    QString::fromUtf8(field.metaType().name()),
                    isPrimaryKey,
                    !isPrimaryKey
                        && field.requiredStatus()
                               != QSqlField::Required,
                    isPrimaryKey
                        && singleColumnPrimaryKey,
                    relationByColumn.value(
                        field.name().toLower())));
            }
        }

        annotateRelationships(columns, relations);

        schema.append(QVariantMap{
            {"name", table},
            {"columns", columns},
            {"relations", relations},
            {"proposed", false}
        });
    }

    return schema;
}

QVariantList DatabaseManager::buildSchemaDiagramForTables(
    const QStringList& tableNames)
{
    const QVariantList completeSchema =
        buildSchemaDiagram();
    QVariantList filteredSchema;

    for (const QString& tableName : tableNames)
    {
        const int index =
            tableIndex(completeSchema, tableName);
        if (index >= 0)
            filteredSchema.append(completeSchema.at(index));
    }

    return filteredSchema;
}

QStringList DatabaseManager::migrationTargetTableNames(
    const QString& migrationSql) const
{
    QStringList tableNames;
    for (const QVariant& tableValue :
         parseCreatedTables(migrationSql))
    {
        const QString tableName =
            tableValue.toMap().value("name").toString();
        if (!tableName.isEmpty()
            && !tableNames.contains(
                tableName,
                Qt::CaseInsensitive))
        {
            tableNames.append(tableName);
        }
    }

    return tableNames;
}

QVariantList DatabaseManager::buildSchemaDiagramWithMigration(
    const QString& migrationSql)
{
    if (!isValidMigrationSql(migrationSql))
        return {};

    const QVariantList proposedTables =
        parseCreatedTables(migrationSql);
    const QVariantList currentSchema =
        buildSchemaDiagram();
    QVariantList normalizedSchema;

    for (const QVariant& proposedTable : proposedTables)
    {
        const QString name =
            proposedTable.toMap().value("name").toString();
        const int existingIndex =
            tableIndex(currentSchema, name);

        if (existingIndex >= 0)
            normalizedSchema.append(currentSchema.at(existingIndex));
        else
            normalizedSchema.append(proposedTable);
    }

    return normalizedSchema.isEmpty()
               ? currentSchema
               : normalizedSchema;
}

bool DatabaseManager::isValidSql(
    const QString& sql)
{
    const QString trimmedSql =
        sql.trimmed();

    if (trimmedSql.isEmpty())
        return false;

    const QRegularExpression destructiveStatement(
        "\\b(DROP|DELETE|TRUNCATE|UPDATE|INSERT|REPLACE|ATTACH|DETACH|VACUUM)\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression createTableStatement(
        "^CREATE\\s+TABLE\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression createIndexStatement(
        "^CREATE\\s+(UNIQUE\\s+)?INDEX\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression alterAddColumnStatement(
        "^ALTER\\s+TABLE\\b[\\s\\S]*\\bADD\\s+COLUMN\\b",
        QRegularExpression::CaseInsensitiveOption);

    bool hasValidStatement = false;

    const QStringList statements =
        trimmedSql.split(
            ";",
            Qt::SkipEmptyParts);

    for (const QString& statement : statements)
    {
        const QString compactStatement =
            statement.trimmed().simplified();

        if (compactStatement.isEmpty())
            continue;

        if (destructiveStatement.match(compactStatement).hasMatch())
            return false;

        const bool isAllowedSchemaStatement =
            createTableStatement.match(compactStatement).hasMatch()
            || createIndexStatement.match(compactStatement).hasMatch()
            || alterAddColumnStatement.match(compactStatement).hasMatch();

        if (!isAllowedSchemaStatement)
            return false;

        hasValidStatement = true;
    }

    return hasValidStatement;
}

bool DatabaseManager::isValidMigrationSql(
    const QString& sql) const
{
    if (sql.trimmed().isEmpty())
        return false;

    const QRegularExpression forbiddenStatement(
        "\\b(DROP|DELETE|TRUNCATE|UPDATE|REPLACE|ATTACH|DETACH|VACUUM|BEGIN|COMMIT|ROLLBACK)\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression alterDropStatement(
        "^ALTER\\s+TABLE\\b[\\s\\S]*\\bDROP\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression createTableStatement(
        "^CREATE\\s+TABLE\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression createIndexStatement(
        "^CREATE\\s+(UNIQUE\\s+)?INDEX\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression insertSelectStatement(
        "^INSERT(?:\\s+OR\\s+IGNORE|\\s+IGNORE)?\\s+INTO\\b[\\s\\S]*\\bSELECT\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression commonTableExpressionInsertSelectStatement(
        "^WITH(?:\\s+RECURSIVE)?\\b[\\s\\S]*\\bINSERT(?:\\s+OR\\s+IGNORE|\\s+IGNORE)?\\s+INTO\\b[\\s\\S]*\\bSELECT\\b",
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression alterSafeStatement(
        "^ALTER\\s+TABLE\\b[\\s\\S]*\\b(ADD|RENAME)\\b",
        QRegularExpression::CaseInsensitiveOption);

    bool hasStatement = false;

    for (QString statement : sqlStatements(sql))
    {
        statement =
            stripLeadingSqlComments(statement);

        const QString compact =
            statement.simplified();

        if (compact.isEmpty())
            continue;

        if (forbiddenStatement.match(compact).hasMatch()
            || alterDropStatement.match(compact).hasMatch())
        {
            return false;
        }

        const bool allowed =
            createTableStatement.match(compact).hasMatch()
            || createIndexStatement.match(compact).hasMatch()
            || insertSelectStatement.match(compact).hasMatch()
            || commonTableExpressionInsertSelectStatement.match(compact).hasMatch()
            || alterSafeStatement.match(compact).hasMatch();

        if (!allowed)
            return false;

        hasStatement = true;
    }

    return hasStatement;
}

bool DatabaseManager::validateMigrationPreview(
    const QString& migrationSql,
    const QStringList& sourceTableNames)
{
    if (!isValidMigrationSql(migrationSql))
    {
        m_lastError =
            "Migration contains unsupported or destructive SQL.";
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::database(m_dataBaseConnectionName);
    if (!db.isOpen())
    {
        m_lastError = "Database is not open.";
        return false;
    }

    const QVariantList sourceSchema =
        sourceTableNames.isEmpty()
            ? buildSchemaDiagram()
            : buildSchemaDiagramForTables(
                  sourceTableNames);
    const QVariantList targetTables =
        parseCreatedTables(migrationSql);
    QStringList structureErrors;
    const QString unkeyedTable =
        firstUnkeyedCreatedTable(migrationSql);
    if (!unkeyedTable.isEmpty())
    {
        structureErrors.append(
            "Created table '" + unkeyedTable
            + "' has no primary or unique key.");
    }
    const QString associationError =
        identifierAssociationError(
            sourceSchema,
            targetTables);
    if (!associationError.isEmpty())
        structureErrors.append(associationError);
    structureErrors.append(
        migrationCopyErrors(
            sourceSchema,
            targetTables,
            migrationSql));
    if (!structureErrors.isEmpty())
    {
        m_lastError = structureErrors.join(" ");
        return false;
    }
    const bool sourceContainsRows =
        schemaHasRows(db, sourceSchema);

    const QString incompatibleFeature =
        incompatibleMigrationFeature(
            db.driverName(),
            migrationSql);
    if (!incompatibleFeature.isEmpty())
    {
        m_lastError =
            "SQL feature '"
            + incompatibleFeature
            + "' is incompatible with "
            + db.driverName()
            + ".";
        return false;
    }

    if (db.driverName() == "QSQLITE")
    {
        QTemporaryDir previewDirectory;
        if (!previewDirectory.isValid())
        {
            m_lastError =
                "Could not create an isolated SQLite normalization preview directory.";
            return false;
        }

        const QString previewPath =
            previewDirectory.filePath(
                "normalization-preview.sqlite");
        QString escapedPreviewPath = previewPath;
        escapedPreviewPath.replace("'", "''");

        QSqlQuery snapshotQuery(db);
        if (!snapshotQuery.exec(
                "VACUUM INTO '"
                + escapedPreviewPath
                + "'"))
        {
            m_lastError =
                "Could not create an isolated SQLite normalization preview: "
                + snapshotQuery.lastError().text();
            return false;
        }

        const QString previewConnectionName =
            "proteus_normalization_preview_"
            + QUuid::createUuid().toString(
                QUuid::WithoutBraces);
        bool previewValid = false;

        {
            QSqlDatabase previewDb =
                QSqlDatabase::addDatabase(
                    "QSQLITE",
                    previewConnectionName);
            previewDb.setDatabaseName(previewPath);

            if (!previewDb.open())
            {
                m_lastError =
                    "Could not open the isolated SQLite normalization preview: "
                    + previewDb.lastError().text();
            }
            else if (!previewDb.transaction())
            {
                m_lastError =
                    "Could not start the isolated SQLite normalization preview: "
                    + previewDb.lastError().text();
            }
            else
            {
                previewValid = true;

                for (QString statement :
                     sqlStatements(migrationSql))
                {
                    statement =
                        stripLeadingSqlComments(statement);
                    if (statement.isEmpty())
                        continue;

                    QSqlQuery query(previewDb);
                    if (!query.exec(statement))
                    {
                        m_lastError =
                            "SQL is not executable for QSQLITE on the isolated preview: "
                            + query.lastError().text();
                        previewValid = false;
                        break;
                    }
                }

                if (previewValid)
                {
                    const QString emptyTarget =
                        sourceContainsRows
                            ? firstEmptyCreatedTable(
                                  previewDb,
                                  targetTables)
                            : QString();
                    if (!emptyTarget.isEmpty())
                    {
                        m_lastError =
                            "Created table '" + emptyTarget
                            + "' contains no rows after the data copy.";
                        previewValid = false;
                    }
                }

                if (previewValid)
                {
                    QSqlQuery foreignKeyCheck(
                        "PRAGMA foreign_key_check",
                        previewDb);
                    if (foreignKeyCheck.next())
                    {
                        m_lastError =
                            "Foreign key validation failed in the isolated SQLite preview.";
                        previewValid = false;
                    }
                }

                if (!previewDb.rollback())
                {
                    m_lastError =
                        "Could not roll back the isolated SQLite normalization preview: "
                        + previewDb.lastError().text();
                    previewValid = false;
                }
            }

            previewDb.close();
        }

        QSqlDatabase::removeDatabase(
            previewConnectionName);

        if (previewValid)
            m_lastError.clear();

        return previewValid;
    }

    const bool supportsRollbackValidation =
        db.driverName().startsWith("QPSQL");
    if (!supportsRollbackValidation)
    {
        m_lastError.clear();
        return true;
    }

    if (!db.transaction())
    {
        m_lastError =
            "Could not start migration preview transaction: "
            + db.lastError().text();
        return false;
    }

    for (QString statement : sqlStatements(migrationSql))
    {
        statement = stripLeadingSqlComments(statement);
        if (statement.isEmpty())
            continue;

        QSqlQuery query(db);
        if (!query.exec(statement))
        {
            m_lastError =
                "SQL is not executable for "
                + db.driverName()
                + ": "
                + query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (sourceContainsRows)
    {
        const QString emptyTarget =
            firstEmptyCreatedTable(db, targetTables);
        if (!emptyTarget.isEmpty())
        {
            m_lastError =
                "Created table '" + emptyTarget
                + "' contains no rows after the data copy.";
            db.rollback();
            return false;
        }
    }

    if (!db.rollback())
    {
        m_lastError =
            "Could not roll back migration preview: "
            + db.lastError().text();
        return false;
    }

    m_lastError.clear();
    return true;
}

bool DatabaseManager::executeMigration(
    const QString& migrationSql,
    const QStringList& sourceTableNames)
{
    if (!isValidMigrationSql(migrationSql))
    {
        m_lastError =
            "Migration contains unsupported or destructive SQL.";
        return false;
    }

    QSqlDatabase db =
        QSqlDatabase::database(
            m_dataBaseConnectionName);

    if (!db.isOpen())
    {
        m_lastError = "Database is not open.";
        return false;
    }

    const QVariantList sourceSchema =
        sourceTableNames.isEmpty()
            ? buildSchemaDiagram()
            : buildSchemaDiagramForTables(
                  sourceTableNames);
    const QVariantList targetTables =
        parseCreatedTables(migrationSql);
    QStringList structureErrors;
    const QString unkeyedTable =
        firstUnkeyedCreatedTable(migrationSql);
    if (!unkeyedTable.isEmpty())
    {
        structureErrors.append(
            "Created table '" + unkeyedTable
            + "' has no primary or unique key.");
    }
    const QString associationError =
        identifierAssociationError(
            sourceSchema,
            targetTables);
    if (!associationError.isEmpty())
        structureErrors.append(associationError);
    structureErrors.append(
        migrationCopyErrors(
            sourceSchema,
            targetTables,
            migrationSql));
    if (!structureErrors.isEmpty())
    {
        m_lastError = structureErrors.join(" ");
        return false;
    }
    const bool sourceContainsRows =
        schemaHasRows(db, sourceSchema);

    if (!db.transaction())
    {
        m_lastError =
            "Could not start database transaction: "
            + db.lastError().text();
        return false;
    }

    for (QString statement : sqlStatements(migrationSql))
    {
        statement =
            stripLeadingSqlComments(statement);

        if (statement.isEmpty())
            continue;

        QSqlQuery query(db);

        if (!query.exec(statement))
        {
            m_lastError =
                query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (sourceContainsRows)
    {
        const QString emptyTarget =
            firstEmptyCreatedTable(db, targetTables);
        if (!emptyTarget.isEmpty())
        {
            m_lastError =
                "Created table '" + emptyTarget
                + "' contains no rows after the data copy.";
            db.rollback();
            return false;
        }
    }

    if (db.driverName() == "QSQLITE")
    {
        QSqlQuery foreignKeyCheck(
            "PRAGMA foreign_key_check",
            db);

        if (foreignKeyCheck.next())
        {
            m_lastError =
                "Foreign key validation failed after migration.";
            db.rollback();
            return false;
        }
    }

    if (!db.commit())
    {
        m_lastError =
            "Could not commit database migration: "
            + db.lastError().text();
        db.rollback();
        return false;
    }

    m_lastError.clear();
    return true;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

