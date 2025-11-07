/*
    SPDX-FileCopyrightText: 2007-2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbupdater.h"
#include "akonadischema.h"
#include "akonadiserver_debug.h"
#include "datastore.h"
#include "dbconfig.h"
#include "dbinitializer_p.h"
#include "dbintrospector.h"
#include "dbtype.h"
#include "entities.h"
#include "querybuilder.h"
#include "selectquerybuilder.h"

#include "private/dbus_p.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QMetaMethod>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QSqlResult>

using namespace Akonadi;
using namespace Akonadi::Server;

DbUpdater::DbUpdater(const QSqlDatabase &database, const QString &filename)
    : m_database(database)
    , m_filename(filename)
{
}

bool DbUpdater::run()
{
    auto store = DataStore::dataStoreForDatabase(m_database);
    const auto schemaVersions = SchemaVersion::retrieveAll(store);
    if (schemaVersions.empty()) {
        qCDebug(AKONADISERVER_LOG) << "DbUpdater: SchemaVersion not found; skipping schema update";
        return true;
    }
    auto currentVersion = schemaVersions.at(0);

    UpdateSet::Map updates;

    if (!parseUpdateSets(currentVersion.version(), updates)) {
        return false;
    }

    if (updates.isEmpty()) {
        return true;
    }

    // indicate clients this might take a while
    // we can ignore unregistration in error cases, that'll kill the server anyway
    if (!QDBusConnection::sessionBus().registerService(DBus::serviceName(DBus::UpgradeIndicator))) {
        qCCritical(AKONADISERVER_LOG) << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();
    }

    // QMap is sorted, so we should be replaying the changes in correct order
    for (QMap<int, UpdateSet>::ConstIterator it = updates.constBegin(); it != updates.constEnd(); ++it) {
        Q_ASSERT(it.key() > currentVersion.version());
        qCDebug(AKONADISERVER_LOG) << "DbUpdater: update to version:" << it.key() << " mandatory:" << it.value().abortOnFailure;

        bool success = false;
        bool hasTransaction = false;
        if (it.value().complex) { // complex update
            const QString methodName = QStringLiteral("complexUpdate_%1()").arg(it.value().version);
            const int index = metaObject()->indexOfMethod(methodName.toLatin1().constData());
            if (index == -1) {
                success = false;
                qCCritical(AKONADISERVER_LOG) << "Update to version" << it.value().version << "marked as complex, but no implementation is available";
            } else {
                const QMetaMethod method = metaObject()->method(index);
                method.invoke(this, Q_RETURN_ARG(bool, success));
                if (!success) {
                    qCCritical(AKONADISERVER_LOG) << "Update failed";
                }
            }
        } else { // regular update
            success = m_database.transaction();
            if (success) {
                hasTransaction = true;
                const QStringList statements = it.value().statements;
                for (const QString &statement : statements) {
                    QSqlQuery query(m_database);
                    success = query.exec(statement);
                    if (!success) {
                        qCCritical(AKONADISERVER_LOG) << "DBUpdater: query error:" << query.lastError().text() << m_database.lastError().text();
                        qCCritical(AKONADISERVER_LOG) << "Query was: " << statement;
                        qCCritical(AKONADISERVER_LOG) << "Target version was: " << it.key();
                        qCCritical(AKONADISERVER_LOG) << "Mandatory: " << it.value().abortOnFailure;
                    }
                }
            }
        }

        if (success) {
            currentVersion.setVersion(it.key());
            success = currentVersion.update();
        }

        if (!success || (hasTransaction && !m_database.commit())) {
            qCCritical(AKONADISERVER_LOG) << "Failed to commit transaction for database update";
            if (hasTransaction) {
                m_database.rollback();
            }
            if (it.value().abortOnFailure) {
                return false;
            }
        }
    }

    QDBusConnection::sessionBus().unregisterService(DBus::serviceName(DBus::UpgradeIndicator));
    return true;
}

bool DbUpdater::parseUpdateSets(int currentVersion, UpdateSet::Map &updates) const
{
    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCCritical(AKONADISERVER_LOG) << "Unable to open update description file" << m_filename;
        return false;
    }

    QDomDocument document;

    const auto result = document.setContent(&file);
    if (!result) {
        qCCritical(AKONADISERVER_LOG) << "Unable to parse update description file" << m_filename << ":" << result.errorMessage << "at line" << result.errorLine
                                      << "column" << result.errorColumn;
        return false;
    }

    const QDomElement documentElement = document.documentElement();
    if (documentElement.tagName() != QLatin1StringView("updates")) {
        qCCritical(AKONADISERVER_LOG) << "Invalid update description file format";
        return false;
    }

    // iterate over the xml document and extract update information into an UpdateSet
    QDomElement updateElement = documentElement.firstChildElement();
    while (!updateElement.isNull()) {
        if (updateElement.tagName() == QLatin1StringView("update")) {
            const int version = updateElement.attribute(QStringLiteral("version"), QStringLiteral("-1")).toInt();
            if (version <= 0) {
                qCCritical(AKONADISERVER_LOG) << "Invalid version attribute in database update description";
                return false;
            }

            if (updates.contains(version)) {
                qCCritical(AKONADISERVER_LOG) << "Duplicate version attribute in database update description";
                return false;
            }

            if (version <= currentVersion) {
                qCDebug(AKONADISERVER_LOG) << "skipping update" << version;
            } else {
                UpdateSet updateSet;
                updateSet.version = version;
                updateSet.abortOnFailure = (updateElement.attribute(QStringLiteral("abortOnFailure")) == QLatin1StringView("true"));

                QDomElement childElement = updateElement.firstChildElement();
                while (!childElement.isNull()) {
                    if (childElement.tagName() == QLatin1StringView("raw-sql")) {
                        if (updateApplicable(childElement.attribute(QStringLiteral("backends")))) {
                            updateSet.statements << buildRawSqlStatement(childElement);
                        }
                    } else if (childElement.tagName() == QLatin1StringView("complex-update")) {
                        if (updateApplicable(childElement.attribute(QStringLiteral("backends")))) {
                            updateSet.complex = true;
                        }
                    }
                    // TODO: check for generic tags here in the future

                    childElement = childElement.nextSiblingElement();
                }

                if (!updateSet.statements.isEmpty() || updateSet.complex) {
                    updates.insert(version, updateSet);
                }
            }
        }
        updateElement = updateElement.nextSiblingElement();
    }

    return true;
}

bool DbUpdater::updateApplicable(const QString &backends) const
{
    const QStringList matchingBackends = backends.split(u',');

    QString currentBackend;
    switch (DbType::type(m_database)) {
    case DbType::MySQL:
        currentBackend = QStringLiteral("mysql");
        break;
    case DbType::PostgreSQL:
        currentBackend = QStringLiteral("psql");
        break;
    case DbType::Sqlite:
        currentBackend = QStringLiteral("sqlite");
        break;
    case DbType::Unknown:
        return false;
    }

    return matchingBackends.contains(currentBackend);
}

QString DbUpdater::buildRawSqlStatement(const QDomElement &element) const
{
    return element.text().trimmed();
}

bool DbUpdater::complexUpdate_25()
{
    qCDebug(AKONADISERVER_LOG) << "Starting database update to version 25";

    DbType::Type dbType = DbType::type(m_database);
    auto store = DataStore::dataStoreForDatabase(m_database);

    QElapsedTimer ttotal;
    ttotal.start();

    // Recover from possibly failed or interrupted update
    {
        // We don't care if this fails, it just means that there was no failed update
        QSqlQuery query(m_database);
        query.exec(QStringLiteral("ALTER TABLE PartTable_old RENAME TO PartTable"));
    }

    {
        QSqlQuery query(m_database);
        query.exec(QStringLiteral("DROP TABLE IF EXISTS PartTable_new"));
    }

    {
        // Make sure the table is empty, otherwise we get duplicate key error
        QSqlQuery query(m_database);
        if (dbType == DbType::Sqlite) {
            query.exec(QStringLiteral("DELETE FROM PartTypeTable"));
        } else { // MySQL, PostgreSQL
            query.exec(QStringLiteral("TRUNCATE TABLE PartTypeTable"));
        }
    }

    {
        // It appears that more users than expected have the invalid "GID" part in their
        // PartTable, which breaks the migration below (see BKO#331867), so we apply this
        // wanna-be fix to remove the invalid part before we start the actual migration.
        QueryBuilder qb(store, QStringLiteral("PartTable"), QueryBuilder::Delete);
        qb.addValueCondition(QStringLiteral("PartTable.name"), Query::Equals, QLatin1StringView("GID"));
        qb.exec();
    }

    qCDebug(AKONADISERVER_LOG) << "Creating a PartTable_new";
    {
        TableDescription description;
        description.name = QStringLiteral("PartTable_new");

        ColumnDescription idColumn;
        idColumn.name = QStringLiteral("id");
        idColumn.type = QStringLiteral("qint64");
        idColumn.isAutoIncrement = true;
        idColumn.isPrimaryKey = true;
        description.columns << idColumn;

        ColumnDescription pimItemIdColumn;
        pimItemIdColumn.name = QStringLiteral("pimItemId");
        pimItemIdColumn.type = QStringLiteral("qint64");
        pimItemIdColumn.allowNull = false;
        description.columns << pimItemIdColumn;

        ColumnDescription partTypeIdColumn;
        partTypeIdColumn.name = QStringLiteral("partTypeId");
        partTypeIdColumn.type = QStringLiteral("qint64");
        partTypeIdColumn.allowNull = false;
        description.columns << partTypeIdColumn;

        ColumnDescription dataColumn;
        dataColumn.name = QStringLiteral("data");
        dataColumn.type = QStringLiteral("QByteArray");
        description.columns << dataColumn;

        ColumnDescription dataSizeColumn;
        dataSizeColumn.name = QStringLiteral("datasize");
        dataSizeColumn.type = QStringLiteral("qint64");
        dataSizeColumn.allowNull = false;
        description.columns << dataSizeColumn;

        ColumnDescription versionColumn;
        versionColumn.name = QStringLiteral("version");
        versionColumn.type = QStringLiteral("int");
        versionColumn.defaultValue = QStringLiteral("0");
        description.columns << versionColumn;

        ColumnDescription externalColumn;
        externalColumn.name = QStringLiteral("external");
        externalColumn.type = QStringLiteral("bool");
        externalColumn.defaultValue = QStringLiteral("false");
        description.columns << externalColumn;

        DbInitializer::Ptr initializer = DbInitializer::createInstance(m_database);
        const QString queryString = initializer->buildCreateTableStatement(description);

        QSqlQuery query(m_database);
        if (!query.exec(queryString)) {
            qCCritical(AKONADISERVER_LOG) << query.lastError().text();
            return false;
        }
    }

    qCDebug(AKONADISERVER_LOG) << "Migrating part types";
    {
        // Get list of all part names
        QueryBuilder qb(store, QStringLiteral("PartTable"), QueryBuilder::Select);
        qb.setDistinct(true);
        qb.addColumn(QStringLiteral("PartTable.name"));

        if (!qb.exec()) {
            qCCritical(AKONADISERVER_LOG) << qb.query().lastError().text();
            return false;
        }

        // Process them one by one
        auto &query = qb.query();
        while (query.next()) {
            // Split the part name to namespace and name and insert it to PartTypeTable
            const QString partName = query.value(0).toString();
            const QString ns = partName.left(3);
            const QString name = partName.mid(4);

            {
                QueryBuilder qb(store, QStringLiteral("PartTypeTable"), QueryBuilder::Insert);
                qb.setColumnValue(QStringLiteral("ns"), ns);
                qb.setColumnValue(QStringLiteral("name"), name);
                if (!qb.exec()) {
                    qCCritical(AKONADISERVER_LOG) << qb.query().lastError().text();
                    return false;
                }
            }
            qCDebug(AKONADISERVER_LOG) << "\t Moved part type" << partName << "to PartTypeTable";
        }
    }

    qCDebug(AKONADISERVER_LOG) << "Migrating data from PartTable to PartTable_new";
    {
        QSqlQuery query(m_database);
        QString queryString;
        if (dbType == DbType::PostgreSQL) {
            queryString = QStringLiteral(
                "INSERT INTO PartTable_new (id, pimItemId, partTypeId, data, datasize, version, external) "
                "SELECT PartTable.id, PartTable.pimItemId, PartTypeTable.id, PartTable.data, "
                "       PartTable.datasize, PartTable.version, PartTable.external "
                "FROM PartTable "
                "LEFT JOIN PartTypeTable ON "
                "          PartTable.name = CONCAT(PartTypeTable.ns, ':', PartTypeTable.name)");
        } else if (dbType == DbType::MySQL) {
            queryString = QStringLiteral(
                "INSERT INTO PartTable_new (id, pimItemId, partTypeId, data, datasize, version, external) "
                "SELECT PartTable.id, PartTable.pimItemId, PartTypeTable.id, PartTable.data, "
                "PartTable.datasize, PartTable.version, PartTable.external "
                "FROM PartTable "
                "LEFT JOIN PartTypeTable ON PartTable.name = CONCAT(PartTypeTable.ns, ':', PartTypeTable.name)");
        } else if (dbType == DbType::Sqlite) {
            queryString = QStringLiteral(
                "INSERT INTO PartTable_new (id, pimItemId, partTypeId, data, datasize, version, external) "
                "SELECT PartTable.id, PartTable.pimItemId, PartTypeTable.id, PartTable.data, "
                "PartTable.datasize, PartTable.version, PartTable.external "
                "FROM PartTable "
                "LEFT JOIN PartTypeTable ON PartTable.name = PartTypeTable.ns || ':' || PartTypeTable.name");
        }

        if (!query.exec(queryString)) {
            qCCritical(AKONADISERVER_LOG) << query.lastError().text();
            return false;
        }
    }

    qCDebug(AKONADISERVER_LOG) << "Swapping PartTable_new for PartTable";
    {
        // Does an atomic swap

        QSqlQuery query(m_database);

        if (dbType == DbType::PostgreSQL || dbType == DbType::Sqlite) {
            if (dbType == DbType::PostgreSQL) {
                m_database.transaction();
            }

            if (!query.exec(QStringLiteral("ALTER TABLE PartTable RENAME TO PartTable_old"))) {
                qCCritical(AKONADISERVER_LOG) << query.lastError().text();
                m_database.rollback();
                return false;
            }

            // If this fails in SQLite (i.e. without transaction), we can still recover on next start)
            if (!query.exec(QStringLiteral("ALTER TABLE PartTable_new RENAME TO PartTable"))) {
                qCCritical(AKONADISERVER_LOG) << query.lastError().text();
                m_database.rollback();
                return false;
            }

            if (dbType == DbType::PostgreSQL) {
                m_database.commit();
            }
        } else { // MySQL cannot do rename in transaction, but supports atomic renames
            if (!query.exec(QStringLiteral("RENAME TABLE PartTable TO PartTable_old,"
                                           "             PartTable_new TO PartTable"))) {
                qCCritical(AKONADISERVER_LOG) << query.lastError().text();
                return false;
            }
        }
    }

    qCDebug(AKONADISERVER_LOG) << "Removing PartTable_old";
    {
        QSqlQuery query(m_database);
        if (!query.exec(QStringLiteral("DROP TABLE PartTable_old;"))) {
            // It does not matter when this fails, we are successfully migrated
            qCDebug(AKONADISERVER_LOG) << query.lastError().text();
            qCDebug(AKONADISERVER_LOG) << "Not a fatal problem, continuing...";
        }
    }

    // Fine tuning for PostgreSQL
    qCDebug(AKONADISERVER_LOG) << "Final tuning of new PartTable";
    {
        QSqlQuery query(m_database);
        if (dbType == DbType::PostgreSQL) {
            query.exec(QStringLiteral("ALTER TABLE PartTable RENAME CONSTRAINT parttable_new_pkey TO parttable_pkey"));
            query.exec(QStringLiteral("ALTER SEQUENCE parttable_new_id_seq RENAME TO parttable_id_seq"));
            query.exec(QStringLiteral("SELECT setval('parttable_id_seq', MAX(id) + 1) FROM PartTable"));
        } else if (dbType == DbType::MySQL) {
            // 0 will automatically reset AUTO_INCREMENT to SELECT MAX(id) + 1 FROM PartTable
            query.exec(QStringLiteral("ALTER TABLE PartTable AUTO_INCREMENT = 0"));
        }
    }

    qCDebug(AKONADISERVER_LOG) << "Update done in" << ttotal.elapsed() << "ms";

    // Foreign keys and constraints will be reconstructed automatically once
    // all updates are done

    return true;
}

bool DbUpdater::complexUpdate_36()
{
    qCDebug(AKONADISERVER_LOG, "Starting database update to version 36");
    Q_ASSERT(DbType::type(m_database) == DbType::Sqlite);

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA foreign_key_checks=OFF"))) {
        qCCritical(AKONADISERVER_LOG, "Failed to disable foreign key checks!");
        return false;
    }

    const auto hasForeignKeys = [](const TableDescription &desc) {
        return std::any_of(desc.columns.cbegin(), desc.columns.cend(), [](const ColumnDescription &col) {
            return !col.refTable.isEmpty() && !col.refColumn.isEmpty();
        });
    };

    const auto recreateTableWithForeignKeys = [this](const TableDescription &table) -> QPair<bool, QSqlQuery> {
        qCDebug(AKONADISERVER_LOG) << "Updating foreign keys in table" << table.name;

        QSqlQuery query(m_database);

        // Recover from possibly failed or interrupted update
        // We don't care if this fails, it just means that there was no failed update
        query.exec(QStringLiteral("ALTER TABLE %1_old RENAME TO %1").arg(table.name));
        query.exec(QStringLiteral("DROP TABLE %1_new").arg(table.name));

        qCDebug(AKONADISERVER_LOG, "\tCreating table %s_new with foreign keys", qUtf8Printable(table.name));
        {
            const auto initializer = DbInitializer::createInstance(m_database);
            TableDescription copy = table;
            copy.name += QStringLiteral("_new");
            if (!query.exec(initializer->buildCreateTableStatement(copy))) {
                // If this fails we will recover on next start
                return {false, std::move(query)};
            }
        }

        qCDebug(AKONADISERVER_LOG,
                "\tCopying values from %s to %s_new (this may take a very long of time...)",
                qUtf8Printable(table.name),
                qUtf8Printable(table.name));
        if (!query.exec(QStringLiteral("INSERT INTO %1_new SELECT * FROM %1").arg(table.name))) {
            // If this fails, we will recover on next start
            return {false, std::move(query)};
        }

        qCDebug(AKONADISERVER_LOG, "\tSwapping %s_new for %s", qUtf8Printable(table.name), qUtf8Printable(table.name));
        if (!query.exec(QStringLiteral("ALTER TABLE %1 RENAME TO %1_old").arg(table.name))) {
            // If this fails we will recover on next start
            return {false, std::move(query)};
        }

        if (!query.exec(QStringLiteral("ALTER TABLE %1_new RENAME TO %1").arg(table.name))) {
            // If this fails we will recover on next start
            return {false, std::move(query)};
        }

        qCDebug(AKONADISERVER_LOG, "\tRemoving table %s_old", qUtf8Printable(table.name));
        if (!query.exec(QStringLiteral("DROP TABLE %1_old").arg(table.name))) {
            // We don't care if this fails
            qCWarning(AKONADISERVER_LOG, "Failed to DROP TABLE %s (not fatal, update will continue)", qUtf8Printable(table.name));
            qCWarning(AKONADISERVER_LOG, "Error: %s", qUtf8Printable(query.lastError().text()));
        }

        qCDebug(AKONADISERVER_LOG) << "\tOptimizing table %s", qUtf8Printable(table.name);
        if (!query.exec(QStringLiteral("ANALYZE %1").arg(table.name))) {
            // We don't care if this fails
            qCWarning(AKONADISERVER_LOG, "Failed to ANALYZE %s (not fatal, update will continue)", qUtf8Printable(table.name));
            qCWarning(AKONADISERVER_LOG, "Error: %s", qUtf8Printable(query.lastError().text()));
        }

        qCDebug(AKONADISERVER_LOG) << "\tDone";
        return {true, QSqlQuery()};
    };

    AkonadiSchema schema;
    const auto tables = schema.tables();
    for (const auto &table : tables) {
        if (!hasForeignKeys(table)) {
            continue;
        }

        const auto &[ok, query] = recreateTableWithForeignKeys(table);
        if (!ok) {
            qCCritical(AKONADISERVER_LOG, "SQL error when updating table %s", qUtf8Printable(table.name));
            qCCritical(AKONADISERVER_LOG, "Query: %s", qUtf8Printable(query.executedQuery()));
            qCCritical(AKONADISERVER_LOG, "Error: %s", qUtf8Printable(query.lastError().text()));
            return false;
        }
    }

    const auto relations = schema.relations();
    for (const auto &relation : relations) {
        const RelationTableDescription table(relation);
        const auto &[ok, query] = recreateTableWithForeignKeys(table);
        if (!ok) {
            qCCritical(AKONADISERVER_LOG, "SQL error when updating relation table %s", qUtf8Printable(table.name));
            qCCritical(AKONADISERVER_LOG, "Query: %s", qUtf8Printable(query.executedQuery()));
            qCCritical(AKONADISERVER_LOG, "Error: %s", qUtf8Printable(query.lastError().text()));
            return false;
        }
    }

    qCDebug(AKONADISERVER_LOG) << "Running VACUUM to reduce DB size";
    if (!query.exec(QStringLiteral("VACUUM"))) {
        qCWarning(AKONADISERVER_LOG) << "Vacuum failed (not fatal, update will continue)";
        qCWarning(AKONADISERVER_LOG) << "Error:" << query.lastError().text();
    }

    return true;
}

#include "moc_dbupdater.cpp"
