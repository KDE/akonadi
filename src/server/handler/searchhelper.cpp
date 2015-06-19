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
#include "storage/countquerybuilder.h"
#include "entities.h"

#include <private/protocol_p.h>

using namespace Akonadi::Server;

QList<QByteArray> SearchHelper::splitLine(const QByteArray &line)
{
    QList<QByteArray> retval;

    int i, start = 0;
    bool escaped = false;
    for (i = 0; i < line.count(); ++i) {
        if (line[i] == ' ') {
            if (!escaped) {
                retval.append(line.mid(start, i - start));
                start = i + 1;
            }
        } else if (line[i] == '"') {
            if (escaped) {
                escaped = false;
            } else {
                escaped = true;
            }
        }
    }

    retval.append(line.mid(start, i - start));

    return retval;
}

QString SearchHelper::extractMimetype(const QList<QByteArray> &junks, int start)
{
    QString mimeType;

    if (junks.count() <= start) {
        return QString();
    }

    if (junks[start].toUpper() == AKONADI_PARAM_CHARSET) {
        if (junks.count() <= (start + 2)) {
            return QString();
        }
        if (junks[start + 2].toUpper() == AKONADI_PARAM_MIMETYPE) {
            if (junks.count() <= (start + 3)) {
                return QString();
            } else {
                mimeType = QString::fromLatin1(junks[start + 3].toLower());
            }
        }
    } else {
        if (junks[start].toUpper() == AKONADI_PARAM_MIMETYPE) {
            if (junks.count() <= (start + 1)) {
                return QString();
            } else {
                mimeType = QString::fromLatin1(junks[start + 1].toLower());
            }
        }
    }

    if (mimeType.isEmpty()) {
        mimeType = QString::fromLatin1("message/rfc822");
    }

    return mimeType;
}

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
    Q_FOREACH (const QString &mt, mimeTypes) {
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
        Q_FOREACH (const QVector<qint64> &res, candidateCollections) {
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

    for (auto iter = candidateCollections.begin(); iter != candidateCollections.end(); ++iter) {
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
