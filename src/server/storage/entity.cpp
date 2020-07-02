/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Andreas Gungl <a.gungl@gmx.de>           *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "entity.h"
#include "datastore.h"
#include "countquerybuilder.h"

#include <QVariant>
#include <QSqlDatabase>

using namespace Akonadi::Server;

Entity::Entity()
    : m_id(-1)
{
}

Entity::Entity(qint64 id)
    : m_id(id)
{
}

Entity::~Entity()
{
}

qint64 Entity::id() const
{
    return m_id;
}

void Entity::setId(qint64 id)
{
    m_id = id;
}

bool Entity::isValid() const
{
    return m_id != -1;
}

QSqlDatabase Entity::database()
{
    return DataStore::self()->database();
}

int Entity::countImpl(const QString &tableName, const QString &column, const QVariant &value)
{
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return -1;
    }

    CountQueryBuilder builder(tableName);
    builder.addValueCondition(column, Query::Equals, value);

    if (!builder.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Error counting records in table" << tableName;
        return -1;
    }

    return builder.result();
}

bool Entity::removeImpl(const QString &tableName, const QString &column, const QVariant &value)
{
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return false;
    }

    QueryBuilder builder(tableName, QueryBuilder::Delete);
    builder.addValueCondition(column, Query::Equals, value);

    if (!builder.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Error during deleting records from table" << tableName;
        return false;
    }
    return true;
}

bool Entity::relatesToImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId)
{
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return false;
    }

    CountQueryBuilder builder(tableName);
    builder.addValueCondition(leftColumn, Query::Equals, leftId);
    builder.addValueCondition(rightColumn, Query::Equals, rightId);

    if (!builder.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Error during counting records in table" << tableName;
        return false;
    }

    return builder.result() > 0;
}

bool Entity::addToRelationImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId)
{
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return false;
    }

    QueryBuilder qb(tableName, QueryBuilder::Insert);
    qb.setColumnValue(leftColumn, leftId);
    qb.setColumnValue(rightColumn, rightId);
    qb.setIdentificationColumn(QString());

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Error during adding a record to table" << tableName;
        return false;
    }

    return true;
}

bool Entity::removeFromRelationImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId)
{
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return false;
    }

    QueryBuilder builder(tableName, QueryBuilder::Delete);
    builder.addValueCondition(leftColumn, Query::Equals, leftId);
    builder.addValueCondition(rightColumn, Query::Equals, rightId);

    if (!builder.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Error during removing a record from relation table" << tableName;
        return false;
    }

    return true;
}

bool Entity::clearRelationImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 id, RelationSide side)
{
    QSqlDatabase db = database();
    if (!db.isOpen()) {
        return false;
    }

    QueryBuilder builder(tableName, QueryBuilder::Delete);
    switch (side) {
    case Left:
        builder.addValueCondition(leftColumn, Query::Equals, id);
        break;
    case Right:
        builder.addValueCondition(rightColumn, Query::Equals, id);
        break;
    default:
        qFatal("Invalid enum value");
    }
    if (!builder.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Error during clearing relation table" << tableName << "for ID" << id;
        return false;
    }

    return true;
}
