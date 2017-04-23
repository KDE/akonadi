/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2014 by Daniel Vrátil <dvratil@redhat.com>              *
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

#include "searchhelper.h"

#include "entities.h"
#include "storage/countquerybuilder.h"

#include <private/protocol_p.h>

using namespace Akonadi::Server;

static qint64 parentCollectionId(qint64 collectionId)
{
    QueryBuilder qb(Collection::tableName(), QueryBuilder::Select);
    qb.addColumn(Collection::parentIdColumn());
    qb.addValueCondition(Collection::idColumn(), Query::Equals, collectionId);
    if (!qb.exec()) {
        return -1;
    }
    if (!qb.query().next()) {
        return -1;
    }
    return qb.query().value(0).toLongLong();
}

QVector<qint64> SearchHelper::matchSubcollectionsByMimeType(const QVector<qint64> &ancestors, const QStringList &mimeTypes)
{
    // Get all collections with given mime types
    QueryBuilder qb(Collection::tableName(), QueryBuilder::Select);
    qb.setDistinct(true);
    qb.addColumn(Collection::idFullColumnName());
    qb.addColumn(Collection::parentIdFullColumnName());
    qb.addJoin(QueryBuilder::LeftJoin, CollectionMimeTypeRelation::tableName(),
               CollectionMimeTypeRelation::leftFullColumnName(), Collection::idFullColumnName());
    qb.addJoin(QueryBuilder::LeftJoin, MimeType::tableName(),
               CollectionMimeTypeRelation::rightFullColumnName(), MimeType::idFullColumnName());
    Query::Condition cond(Query::Or);
    for (const QString &mt : mimeTypes) {
        cond.addValueCondition(MimeType::nameFullColumnName(), Query::Equals, mt);
    }
    if (!cond.isEmpty()) {
        qb.addCondition(cond);
    }

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to query search collections";
        return QVector<qint64>();
    }

    QMap<qint64 /* parentId */, QVector<qint64> /* collectionIds */> candidateCollections;
    while (qb.query().next()) {
        candidateCollections[qb.query().value(1).toLongLong()].append(qb.query().value(0).toLongLong());
    }

    // If the ancestors list contains root, then return what we got, since everything
    // is sub collection of root
    QVector<qint64> results;
    if (ancestors.contains(0)) {
        for (const QVector<qint64> &res : qAsConst(candidateCollections)) {
            results += res;
        }
        return results;
    }

    // Try to resolve direct descendants
    Q_FOREACH (qint64 ancestor, ancestors) {
        const QVector<qint64> cols = candidateCollections.take(ancestor);
        if (!cols.isEmpty()) {
            results += cols;
        }
    }

    for (auto iter = candidateCollections.begin(), iterEnd = candidateCollections.end() ; iter != iterEnd ; ++iter) {
        // Traverse the collection chain up to root
        qint64 parentId = iter.key();
        while (!ancestors.contains(parentId) && parentId > 0) {
            parentId = parentCollectionId(parentId);
        }
        // Ok, we found a requested ancestor in the parent chain
        if (parentId > 0) {
            results += iter.value();
        }
    }

    return results;
}
