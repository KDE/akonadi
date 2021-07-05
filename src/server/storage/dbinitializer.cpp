/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "dbinitializer.h"
#include "akonadiserver_debug.h"
#include "dbexception.h"
#include "dbinitializer_p.h"
#include "dbtype.h"
#include "entities.h"
#include "schema.h"

#include <QDateTime>
#include <QSqlQuery>
#include <QStringList>

#include <algorithm>

#include <private/tristate_p.h>

using namespace Akonadi::Server;

DbInitializer::Ptr DbInitializer::createInstance(const QSqlDatabase &database, Schema *schema)
{
    DbInitializer::Ptr i;
    switch (DbType::type(database)) {
    case DbType::MySQL:
        i.reset(new DbInitializerMySql(database));
        break;
    case DbType::Sqlite:
        i.reset(new DbInitializerSqlite(database));
        break;
    case DbType::PostgreSQL:
        i.reset(new DbInitializerPostgreSql(database));
        break;
    case DbType::Unknown:
        qCCritical(AKONADISERVER_LOG) << database.driverName() << "backend not supported";
        break;
    }
    i->mSchema = schema;
    return i;
}

DbInitializer::DbInitializer(const QSqlDatabase &database)
    : mDatabase(database)
    , mSchema(nullptr)
    , mTestInterface(nullptr)
{
    m_introspector = DbIntrospector::createInstance(mDatabase);
}

DbInitializer::~DbInitializer()
{
}

bool DbInitializer::run()
{
    try {
        qCInfo(AKONADISERVER_LOG) << "Running DB initializer";

        const auto tables = mSchema->tables();
        for (const TableDescription &table : tables) {
            if (!checkTable(table)) {
                return false;
            }
        }

        const auto relations = mSchema->relations();
        for (const RelationDescription &relation : relations) {
            if (!checkRelation(relation)) {
                return false;
            }
        }

#ifndef DBINITIALIZER_UNITTEST
        // Now finally check and set the generation identifier if necessary
        SchemaVersion version = SchemaVersion::retrieveAll().at(0);
        if (version.generation() == 0) {
            version.setGeneration(QDateTime::currentDateTimeUtc().toTime_t());
            version.update();

            qCDebug(AKONADISERVER_LOG) << "Generation:" << version.generation();
        }
#endif

        qCInfo(AKONADISERVER_LOG) << "DB initializer done";
        return true;
    } catch (const DbException &e) {
        mErrorMsg = QString::fromUtf8(e.what());
    }
    return false;
}

bool DbInitializer::checkTable(const TableDescription &tableDescription)
{
    qCDebug(AKONADISERVER_LOG) << "checking table " << tableDescription.name;

    if (!m_introspector->hasTable(tableDescription.name)) {
        // Get the CREATE TABLE statement for the specific SQL dialect
        const QString createTableStatement = buildCreateTableStatement(tableDescription);
        qCDebug(AKONADISERVER_LOG) << createTableStatement;
        execQuery(createTableStatement);
    } else {
        // Check for every column whether it exists, and add the missing ones
        Q_FOREACH (const ColumnDescription &columnDescription, tableDescription.columns) {
            if (!m_introspector->hasColumn(tableDescription.name, columnDescription.name)) {
                // Don't add the column on update, DbUpdater will add it
                if (columnDescription.noUpdate) {
                    continue;
                }
                // Get the ADD COLUMN statement for the specific SQL dialect
                const QString statement = buildAddColumnStatement(tableDescription, columnDescription);
                qCDebug(AKONADISERVER_LOG) << statement;
                execQuery(statement);
            }
        }

        // NOTE: we do intentionally not delete any columns here, we defer that to the updater,
        // very likely previous columns contain data that needs to be moved to a new column first.
    }

    // Add initial data if table is empty
    if (tableDescription.data.isEmpty()) {
        return true;
    }
    if (m_introspector->isTableEmpty(tableDescription.name)) {
        Q_FOREACH (const DataDescription &dataDescription, tableDescription.data) {
            // Get the INSERT VALUES statement for the specific SQL dialect
            const QString statement = buildInsertValuesStatement(tableDescription, dataDescription);
            qCDebug(AKONADISERVER_LOG) << statement;
            execQuery(statement);
        }
    }

    return true;
}

void DbInitializer::checkForeignKeys(const TableDescription &tableDescription)
{
    try {
        const QVector<DbIntrospector::ForeignKey> existingForeignKeys = m_introspector->foreignKeyConstraints(tableDescription.name);
        Q_FOREACH (const ColumnDescription &column, tableDescription.columns) {
            DbIntrospector::ForeignKey existingForeignKey;
            for (const DbIntrospector::ForeignKey &fk : existingForeignKeys) {
                if (QString::compare(fk.column, column.name, Qt::CaseInsensitive) == 0) {
                    existingForeignKey = fk;
                    break;
                }
            }

            if (!column.refTable.isEmpty() && !column.refColumn.isEmpty()) {
                if (!existingForeignKey.column.isEmpty()) {
                    // there's a constraint on this column, check if it's the correct one
                    if (QString::compare(existingForeignKey.refTable, column.refTable + QLatin1String("table"), Qt::CaseInsensitive) == 0
                        && QString::compare(existingForeignKey.refColumn, column.refColumn, Qt::CaseInsensitive) == 0
                        && QString::compare(existingForeignKey.onUpdate, referentialActionToString(column.onUpdate), Qt::CaseInsensitive) == 0
                        && QString::compare(existingForeignKey.onDelete, referentialActionToString(column.onDelete), Qt::CaseInsensitive) == 0) {
                        continue; // all good
                    }

                    const auto statements = buildRemoveForeignKeyConstraintStatements(existingForeignKey, tableDescription);
                    if (!statements.isEmpty()) {
                        qCDebug(AKONADISERVER_LOG) << "Found existing foreign constraint that doesn't match the schema:" << existingForeignKey.name
                                                   << existingForeignKey.column << existingForeignKey.refTable << existingForeignKey.refColumn;
                        m_removedForeignKeys << statements;
                    }
                }

                const auto statements = buildAddForeignKeyConstraintStatements(tableDescription, column);
                if (statements.isEmpty()) { // not supported
                    return;
                }

                m_pendingForeignKeys << statements;

            } else if (!existingForeignKey.column.isEmpty()) {
                // constraint exists but we don't want one here
                const auto statements = buildRemoveForeignKeyConstraintStatements(existingForeignKey, tableDescription);
                if (!statements.isEmpty()) {
                    qCDebug(AKONADISERVER_LOG) << "Found unexpected foreign key constraint:" << existingForeignKey.name << existingForeignKey.column
                                               << existingForeignKey.refTable << existingForeignKey.refColumn;
                    m_removedForeignKeys << statements;
                }
            }
        }
    } catch (const DbException &e) {
        qCDebug(AKONADISERVER_LOG) << "Fixing foreign key constraints failed:" << e.what();
    }
}

void DbInitializer::checkIndexes(const TableDescription &tableDescription)
{
    // Add indices
    Q_FOREACH (const IndexDescription &indexDescription, tableDescription.indexes) {
        // sqlite3 needs unique index identifiers per db
        const QString indexName = QStringLiteral("%1_%2").arg(tableDescription.name, indexDescription.name);
        if (!m_introspector->hasIndex(tableDescription.name, indexName)) {
            // Get the CREATE INDEX statement for the specific SQL dialect
            m_pendingIndexes << buildCreateIndexStatement(tableDescription, indexDescription);
        }
    }
}

bool DbInitializer::checkRelation(const RelationDescription &relationDescription)
{
    return checkTable(RelationTableDescription(relationDescription));
}

QString DbInitializer::errorMsg() const
{
    return mErrorMsg;
}

bool DbInitializer::updateIndexesAndConstraints()
{
    Q_FOREACH (const TableDescription &table, mSchema->tables()) {
        // Make sure the foreign key constraints are all there
        checkForeignKeys(table);
        checkIndexes(table);
    }
    Q_FOREACH (const RelationDescription &relation, mSchema->relations()) {
        RelationTableDescription relTable(relation);
        checkForeignKeys(relTable);
        checkIndexes(relTable);
    }

    try {
        if (!m_pendingIndexes.isEmpty()) {
            qCDebug(AKONADISERVER_LOG) << "Updating indexes";
            execPendingQueries(m_pendingIndexes);
            m_pendingIndexes.clear();
        }

        if (!m_removedForeignKeys.isEmpty()) {
            qCDebug(AKONADISERVER_LOG) << "Removing invalid foreign key constraints";
            execPendingQueries(m_removedForeignKeys);
            m_removedForeignKeys.clear();
        }

        if (!m_pendingForeignKeys.isEmpty()) {
            qCDebug(AKONADISERVER_LOG) << "Adding new foreign key constraints";
            execPendingQueries(m_pendingForeignKeys);
            m_pendingForeignKeys.clear();
        }
    } catch (const DbException &e) {
        qCCritical(AKONADISERVER_LOG) << "Updating index failed: " << e.what();
        return false;
    }

    qCDebug(AKONADISERVER_LOG) << "Indexes successfully created";
    return true;
}

void DbInitializer::execPendingQueries(const QStringList &queries)
{
    for (const QString &statement : queries) {
        qCDebug(AKONADISERVER_LOG) << statement;
        execQuery(statement);
    }
}

QString DbInitializer::sqlType(const ColumnDescription &col, int size) const
{
    Q_UNUSED(size)
    if (col.type == QLatin1String("int")) {
        return QStringLiteral("INTEGER");
    }
    if (col.type == QLatin1String("qint64")) {
        return QStringLiteral("BIGINT");
    }
    if (col.type == QLatin1String("QString")) {
        return QStringLiteral("TEXT");
    }
    if (col.type == QLatin1String("QByteArray")) {
        return QStringLiteral("LONGBLOB");
    }
    if (col.type == QLatin1String("QDateTime")) {
        return QStringLiteral("TIMESTAMP");
    }
    if (col.type == QLatin1String("bool")) {
        return QStringLiteral("BOOL");
    }
    if (col.isEnum) {
        return QStringLiteral("TINYINT");
    }

    qCCritical(AKONADISERVER_LOG) << "Invalid type" << col.type;
    Q_ASSERT(false);
    return QString();
}

QString DbInitializer::sqlValue(const ColumnDescription &col, const QString &value) const
{
    if (col.type == QLatin1String("QDateTime") && value == QLatin1String("QDateTime::currentDateTimeUtc()")) {
        return QStringLiteral("CURRENT_TIMESTAMP");
    } else if (col.isEnum) {
        return QString::number(col.enumValueMap[value]);
    }
    return value;
}

QString DbInitializer::buildAddColumnStatement(const TableDescription &tableDescription, const ColumnDescription &columnDescription) const
{
    return QStringLiteral("ALTER TABLE %1 ADD COLUMN %2").arg(tableDescription.name, buildColumnStatement(columnDescription, tableDescription));
}

QString DbInitializer::buildCreateIndexStatement(const TableDescription &tableDescription, const IndexDescription &indexDescription) const
{
    const QString indexName = QStringLiteral("%1_%2").arg(tableDescription.name, indexDescription.name);
    QStringList columns;
    if (indexDescription.sort.isEmpty()) {
        columns = indexDescription.columns;
    } else {
        columns.reserve(indexDescription.columns.count());
        std::transform(indexDescription.columns.cbegin(),
                       indexDescription.columns.cend(),
                       std::back_insert_iterator<QStringList>(columns),
                       [&indexDescription](const QString &column) {
                           return QStringLiteral("%1 %2").arg(column, indexDescription.sort);
                       });
    }

    return QStringLiteral("CREATE %1 INDEX %2 ON %3 (%4)")
        .arg(indexDescription.isUnique ? QStringLiteral("UNIQUE") : QString(), indexName, tableDescription.name, columns.join(QLatin1Char(',')));
}

QStringList DbInitializer::buildAddForeignKeyConstraintStatements(const TableDescription &table, const ColumnDescription &column) const
{
    Q_UNUSED(table)
    Q_UNUSED(column)
    return {};
}

QStringList DbInitializer::buildRemoveForeignKeyConstraintStatements(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const
{
    Q_UNUSED(fk)
    Q_UNUSED(table)
    return {};
}

QString DbInitializer::buildReferentialAction(ColumnDescription::ReferentialAction onUpdate, ColumnDescription::ReferentialAction onDelete)
{
    return QLatin1String("ON UPDATE ") + referentialActionToString(onUpdate) + QLatin1String(" ON DELETE ") + referentialActionToString(onDelete);
}

QString DbInitializer::referentialActionToString(ColumnDescription::ReferentialAction action)
{
    switch (action) {
    case ColumnDescription::Cascade:
        return QStringLiteral("CASCADE");
    case ColumnDescription::Restrict:
        return QStringLiteral("RESTRICT");
    case ColumnDescription::SetNull:
        return QStringLiteral("SET NULL");
    }

    Q_ASSERT(!"invalid referential action enum!");
    return QString();
}

QString DbInitializer::buildPrimaryKeyStatement(const TableDescription &table)
{
    QStringList cols;
    for (const ColumnDescription &column : std::as_const(table.columns)) {
        if (column.isPrimaryKey) {
            cols.push_back(column.name);
        }
    }
    return QLatin1String("PRIMARY KEY (") + cols.join(QLatin1String(", ")) + QLatin1Char(')');
}

void DbInitializer::execQuery(const QString &queryString)
{
    // if ( Q_UNLIKELY( mTestInterface ) ) { Qt 4.7 has no Q_UNLIKELY yet
    if (mTestInterface) {
        mTestInterface->execStatement(queryString);
        return;
    }

    QSqlQuery query(mDatabase);
    if (!query.exec(queryString)) {
        throw DbException(query);
    }
}

void DbInitializer::setTestInterface(TestInterface *interface)
{
    mTestInterface = interface;
}

void DbInitializer::setIntrospector(const DbIntrospector::Ptr &introspector)
{
    m_introspector = introspector;
}
