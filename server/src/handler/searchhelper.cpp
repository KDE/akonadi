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

#include <libs/protocol_p.h>

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

QVector<qint64> SearchHelper::listCollectionsRecursive(const QVector<qint64> &ancestors, const QStringList &mimeTypes)
{
    QVector<qint64> recursiveChildren;
    Q_FOREACH (qint64 ancestor, ancestors) {
        QVector<qint64> searchChildren;

        {   // Free the query before entering recursion to prevent too many opened connections

            Query::Condition mimeTypeCondition;
            mimeTypeCondition.addColumnCondition(CollectionMimeTypeRelation::rightFullColumnName(), Query::Equals, MimeType::idFullColumnName());
            // Exclude top-level collections and collections that cannot have items!
            mimeTypeCondition.addValueCondition(MimeType::nameFullColumnName(), Query::NotEquals, QLatin1String("inode/directory"));
            if (!mimeTypes.isEmpty()) {
                mimeTypeCondition.addValueCondition(MimeType::nameFullColumnName(), Query::In, mimeTypes);
            }

            CountQueryBuilder qb(Collection::tableName(), MimeType::nameFullColumnName(), CountQueryBuilder::All);
            qb.addColumn(Collection::idFullColumnName());
            qb.addJoin(QueryBuilder::LeftJoin, CollectionMimeTypeRelation::tableName(), CollectionMimeTypeRelation::leftFullColumnName(), Collection::idFullColumnName());
            qb.addJoin(QueryBuilder::LeftJoin, MimeType::tableName(), mimeTypeCondition);
            if (ancestor == 0) {
                qb.addValueCondition(Collection::parentIdFullColumnName(), Query::Is, QVariant());
            } else {
                // Also include current ancestor's result, so that we know whether we should search in the ancestor too
                Query::Condition idCond(Query::Or);
                idCond.addValueCondition(Collection::parentIdFullColumnName(), Query::Equals, ancestor);
                idCond.addValueCondition(Collection::idFullColumnName(), Query::Equals, ancestor);
                qb.addCondition(idCond);
            }
            qb.addValueCondition(Collection::isVirtualFullColumnName(), Query::Equals, false);
            qb.addGroupColumn(Collection::idFullColumnName());
            qb.exec();

            QSqlQuery query = qb.query();
            while (query.next()) {
                const qint64 id = query.value(1).toLongLong();
                // Don't add ancestor into search children, we are resolving it right now
                if (id != ancestor) {
                    searchChildren << id;
                }
                if (query.value(0).toInt() > 0) {     // count( mimeTypeTable.name ) > 0
                    recursiveChildren << id;
                }
            }
        }
        if (!searchChildren.isEmpty()) {
            recursiveChildren << listCollectionsRecursive(searchChildren, mimeTypes);
        }
    }

    return recursiveChildren;
}
