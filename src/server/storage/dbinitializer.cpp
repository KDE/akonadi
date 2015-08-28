/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2012 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "dbinitializer.h"
#include "dbinitializer_p.h"
#include "querybuilder.h"
#include "dbexception.h"
#include "shared/akdebug.h"
#include "schema.h"
#include "entity.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtSql/QSqlField>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

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
        akFatal() << database.driverName() << "backend not supported";
        break;
    }
    i->mSchema = schema;
    return i;
}

DbInitializer::DbInitializer(const QSqlDatabase &database)
    : mDatabase(database)
    , mSchema(0)
    , mTestInterface(0)
    , m_noForeignKeyContraints(false)
{
    m_introspector = DbIntrospector::createInstance(mDatabase);
}

DbInitializer::~DbInitializer()
{
}

bool DbInitializer::run()
{
    try {
        akDebug() << "DbInitializer::run()";

        Q_FOREACH (const TableDescription &table, mSchema->tables()) {
            if (!checkTable(table)) {
                return false;
            }
        }

        Q_FOREACH (const RelationDescription &relation, mSchema->relations()) {
            if (!checkRelation(relation)) {
                return false;
            }
        }

        akDebug() << "DbInitializer::run() done";
        return true;
    } catch (const DbException &e) {
        mErrorMsg = QString::fromUtf8(e.what());
    }
    return false;
}

bool DbInitializer::checkTable(const TableDescription &tableDescription)
{
    akDebug() << "checking table " << tableDescription.name;

    if (!m_introspector->hasTable(tableDescription.name)) {
        // Get the CREATE TABLE statement for the specific SQL dialect
        const QString createTableStatement = buildCreateTableStatement(tableDescription);
        akDebug() << createTableStatement;
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
                akDebug() << statement;
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
            akDebug() << statement;
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
            Q_FOREACH (const DbIntrospector::ForeignKey &fk, existingForeignKeys) {
                if (QString::compare(fk.column, column.name, Qt::CaseInsensitive) == 0) {
                    existingForeignKey = fk;
                    break;
                }
            }

            if (!column.refTable.isEmpty() && !column.refColumn.isEmpty()) {
                if (!existingForeignKey.column.isEmpty()) {
                    // there's a constraint on this column, check if it's the correct one
                    if (QString::compare(existingForeignKey.refTable, column.refTable + QLatin1Literal("table"), Qt::CaseInsensitive) == 0
                        && QString::compare(existingForeignKey.refColumn, column.refColumn, Qt::CaseInsensitive) == 0
                        && QString::compare(existingForeignKey.onUpdate, referentialActionToString(column.onUpdate), Qt::CaseInsensitive) == 0
                        && QString::compare(existingForeignKey.onDelete, referentialActionToString(column.onDelete), Qt::CaseInsensitive) == 0) {
                        continue; // all good
                    }

                    const QString statement = buildRemoveForeignKeyConstraintStatement(existingForeignKey, tableDescription);
                    if (!statement.isEmpty()) {
                        akDebug() << "Found existing foreign constraint that doesn't match the schema:" << existingForeignKey.name
                                  << existingForeignKey.column << existingForeignKey.refTable << existingForeignKey.refColumn;
                        m_removedForeignKeys << statement;
                    }
                }

                const QString statement = buildAddForeignKeyConstraintStatement(tableDescription, column);
                if (statement.isEmpty()) {   // not supported
                    m_noForeignKeyContraints = true;
                    return;
                }

                m_pendingForeignKeys << statement;

            } else if (!existingForeignKey.column.isEmpty()) {
                // constraint exists but we don't want one here
                const QString statement = buildRemoveForeignKeyConstraintStatement(existingForeignKey, tableDescription);
                if (!statement.isEmpty()) {
                    akDebug() << "Found unexpected foreign key constraint:" << existingForeignKey.name << existingForeignKey.column
                              << existingForeignKey.refTable << existingForeignKey.refColumn;
                    m_removedForeignKeys << statement;
                }
            }
        }
    } catch (const DbException &e) {
        akDebug() << "Fixing foreign key constraints failed:" << e.what();
        // we ignore this since foreign keys are only used for optimizations (not all backends support them anyway)
        m_noForeignKeyContraints = true;
    }
}

void DbInitializer::checkIndexes(const TableDescription &tableDescription)
{
    // Add indices
    Q_FOREACH (const IndexDescription &indexDescription, tableDescription.indexes) {
        // sqlite3 needs unique index identifiers per db
        const QString indexName = QStringLiteral("%1_%2").arg(tableDescription.name).arg(indexDescription.name);
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

bool DbInitializer::hasForeignKeyConstraints() const
{
    return !m_noForeignKeyContraints;
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
            akDebug() << "Updating indexes";
            execPendingQueries(m_pendingIndexes);
            m_pendingIndexes.clear();
        }

        if (!m_removedForeignKeys.isEmpty()) {
            akDebug() << "Removing invalid foreign key constraints";
            execPendingQueries(m_removedForeignKeys);
            m_removedForeignKeys.clear();
        }

        if (!m_pendingForeignKeys.isEmpty()) {
            akDebug() << "Adding new foreign key constraints";
            execPendingQueries(m_pendingForeignKeys);
            m_pendingForeignKeys.clear();
        }
    } catch (const DbException &e) {
        akDebug() << "Updating index failed: " << e.what();
        return false;
    }

    akDebug() << "Indexes successfully created";
    return true;
}

void DbInitializer::execPendingQueries(const QStringList &queries)
{
    Q_FOREACH (const QString &statement, queries) {
        akDebug() << statement;
        execQuery(statement);
    }
}

QString DbInitializer::sqlType(const QString &type, int size) const
{
    Q_UNUSED(size);
    if (type == QLatin1String("int")) {
        return QStringLiteral("INTEGER");
    }
    if (type == QLatin1String("qint64")) {
        return QStringLiteral("BIGINT");
    }
    if (type == QLatin1String("QString")) {
        return QStringLiteral("TEXT");
    }
    if (type == QLatin1String("QByteArray")) {
        return QStringLiteral("LONGBLOB");
    }
    if (type == QLatin1String("QDateTime")) {
        return QStringLiteral("TIMESTAMP");
    }
    if (type == QLatin1String("bool")) {
        return QStringLiteral("BOOL");
    }
    if (type == QLatin1String("Tristate")) {
        return QStringLiteral("TINYINT");
    }

    akDebug() << "Invalid type" << type;
    Q_ASSERT(false);
    return QString();
}

QString DbInitializer::sqlValue(const QString &type, const QString &value) const
{
    if (type == QLatin1String("QDateTime") && value == QLatin1String("QDateTime::currentDateTime()")) {
        return QStringLiteral("CURRENT_TIMESTAMP");
    }
    if (type == QLatin1String("Tristate")) {
        if (value == QLatin1String("False")) {
            return QString::number((int)Akonadi::Tristate::False);
        }
        if (value == QLatin1String("True")) {
            return QString::number((int)Akonadi::Tristate::True);
        }
        return QString::number((int)Akonadi::Tristate::Undefined);
    }

    return value;
}

QString DbInitializer::buildAddColumnStatement(const TableDescription &tableDescription, const ColumnDescription &columnDescription) const
{
    return QStringLiteral("ALTER TABLE %1 ADD COLUMN %2").arg(tableDescription.name, buildColumnStatement(columnDescription, tableDescription));
}

QString DbInitializer::buildCreateIndexStatement(const TableDescription &tableDescription, const IndexDescription &indexDescription) const
{
    const QString indexName = QStringLiteral("%1_%2").arg(tableDescription.name).arg(indexDescription.name);
    return QStringLiteral("CREATE %1 INDEX %2 ON %3 (%4)")
           .arg(indexDescription.isUnique ? QStringLiteral("UNIQUE") : QString())
           .arg(indexName)
           .arg(tableDescription.name)
           .arg(indexDescription.columns.join(QStringLiteral(",")));
}

QString DbInitializer::buildAddForeignKeyConstraintStatement(const TableDescription &table, const ColumnDescription &column) const
{
    Q_UNUSED(table);
    Q_UNUSED(column);
    return QString();
}

QString DbInitializer::buildRemoveForeignKeyConstraintStatement(const DbIntrospector::ForeignKey &fk, const TableDescription &table) const
{
    Q_UNUSED(fk);
    Q_UNUSED(table);
    return QString();
}

QString DbInitializer::buildReferentialAction(ColumnDescription::ReferentialAction onUpdate, ColumnDescription::ReferentialAction onDelete)
{
    return QLatin1Literal("ON UPDATE ") + referentialActionToString(onUpdate)
           + QLatin1Literal(" ON DELETE ") + referentialActionToString(onDelete);
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
    Q_FOREACH (const ColumnDescription &column, table.columns) {
        if (column.isPrimaryKey) {
            cols.push_back(column.name);
        }
    }
    return QLatin1Literal("PRIMARY KEY (") + cols.join(QStringLiteral(", ")) + QLatin1Char(')');
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
