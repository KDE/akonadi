/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "handlerhelper.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionstatistics.h"
#include "storage/queryhelper.h"
#include "storage/collectionqueryhelper.h"
#include "commandcontext.h"
#include "handler.h"
#include "connection.h"
#include "utils.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>
#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

Collection HandlerHelper::collectionFromIdOrName(const QByteArray &id)
{
    // id is a number
    bool ok = false;
    qint64 collectionId = id.toLongLong(&ok);
    if (ok) {
        return Collection::retrieveById(collectionId);
    }

    // id is a path
    QString path = QString::fromUtf8(id);   // ### should be UTF-7 for real IMAP compatibility

    const QStringList pathParts = path.split(QLatin1Char('/'), QString::SkipEmptyParts);
    Collection col;
    Q_FOREACH (const QString &part, pathParts) {
        SelectQueryBuilder<Collection> qb;
        qb.addValueCondition(Collection::nameColumn(), Query::Equals, part);
        if (col.isValid()) {
            qb.addValueCondition(Collection::parentIdColumn(), Query::Equals, col.id());
        } else {
            qb.addValueCondition(Collection::parentIdColumn(), Query::Is, QVariant());
        }
        if (!qb.exec()) {
            return Collection();
        }
        Collection::List list = qb.result();
        if (list.count() != 1) {
            return Collection();
        }
        col = list.first();
    }
    return col;
}

QString HandlerHelper::pathForCollection(const Collection &col)
{
    QStringList parts;
    Collection current = col;
    while (current.isValid()) {
        parts.prepend(current.name());
        current = current.parent();
    }
    return parts.join(QStringLiteral("/"));
}

Protocol::CachePolicy HandlerHelper::cachePolicyResponse(const Collection& col)
{
    Protocol::CachePolicy cachePolicy;
    cachePolicy.setInherit(col.cachePolicyInherit());
    cachePolicy.setCacheTimeout(col.cachePolicyCacheTimeout());
    cachePolicy.setCheckInterval(col.cachePolicyCheckInterval());
    cachePolicy.setLocalParts(col.cachePolicyLocalParts().split(QLatin1Char(' ')));
    cachePolicy.setSyncOnDemand(col.cachePolicySyncOnDemand());
    return cachePolicy;
}

Protocol::FetchCollectionsResponse HandlerHelper::fetchCollectionsResponse(const Collection &col)
{
    QStringList mimeTypes;
    mimeTypes.reserve(col.mimeTypes().size());
    Q_FOREACH (const MimeType &mt, col.mimeTypes()) {
        mimeTypes << mt.name();
    }

    return fetchCollectionsResponse(col, col.attributes(), false, 0, QStack<Collection>(),
                                    QStack<CollectionAttribute::List>(), false, mimeTypes);
}

Protocol::FetchCollectionsResponse HandlerHelper::fetchCollectionsResponse(const Collection &col,
                                                                           const CollectionAttribute::List &attrs,
                                                                           bool includeStatistics,
                                                                           int ancestorDepth,
                                                                           const QStack<Collection> &ancestors,
                                                                           const QStack<CollectionAttribute::List> &ancestorAttributes,
                                                                           bool isReferenced,
                                                                           const QStringList &mimeTypes)
{
    Protocol::FetchCollectionsResponse response(col.id());
    response.setParentId(col.parentId());
    response.setName(col.name());
    response.setMimeTypes(mimeTypes);
    response.setRemoteId(col.remoteId());
    response.setRemoteRevision(col.remoteRevision());
    response.setResource(col.resource().name());
    response.setIsVirtual(col.isVirtual());

    if (includeStatistics) {
        const CollectionStatistics::Statistics stats = CollectionStatistics::self()->statistics(col);
        if (stats.count > -1) {
            Protocol::FetchCollectionStatsResponse statsResponse(stats.count,
                                                                 stats.count - stats.read,
                                                                 stats.size);
            response.setStatistics(statsResponse);
        }
    }

    if (!col.queryString().isEmpty()) {
        response.setSearchQuery(col.queryString());
        QVector<qint64> searchCols;
        const QStringList searchColIds = col.queryCollections().split(QLatin1Char(' '));
        searchCols.reserve(searchColIds.size());
        Q_FOREACH (const QString &searchColId, searchColIds) {
            searchCols << searchColId.toLongLong();
        }
        response.setSearchCollections(searchCols);
    }

    Protocol::CachePolicy cachePolicy = cachePolicyResponse(col);
    response.setCachePolicy(cachePolicy);

    if (ancestorDepth) {
        QVector<Protocol::Ancestor> ancestorList
            = HandlerHelper::ancestorsResponse(ancestorDepth, ancestors, ancestorAttributes);
        response.setAncestors(ancestorList);
    }

    response.setReferenced(isReferenced);
    response.setEnabled(col.enabled());
    response.setDisplayPref(col.displayPref());
    response.setSyncPref(col.syncPref());
    response.setIndexPref(col.indexPref());

    QMap<QByteArray, QByteArray> ra;
    for (const CollectionAttribute &attr : attrs) {
        ra.insert(attr.type(), attr.value());
    }
    response.setAttributes(ra);

    return response;
}

QVector<Protocol::Ancestor> HandlerHelper::ancestorsResponse(int ancestorDepth,
                                                             const QStack<Collection> &_ancestors,
                                                             const QStack<CollectionAttribute::List> &_ancestorsAttributes)
{
    QVector<Protocol::Ancestor> rv;
    if (ancestorDepth  > 0) {
        QStack<Collection> ancestors(_ancestors);
        QStack<CollectionAttribute::List> ancestorAttributes(_ancestorsAttributes);
        for (int i = 0; i < ancestorDepth; ++i) {
            if (ancestors.isEmpty()) {
                rv << Protocol::Ancestor(0);
                break;
            }
            const Collection c = ancestors.pop();
            Protocol::Ancestor a(c.id());
            a.setRemoteId(c.remoteId());
            a.setName(c.name());
            if (!ancestorAttributes.isEmpty()) {
                QMap<QByteArray, QByteArray> attrs;
                Q_FOREACH (const CollectionAttribute &attr, ancestorAttributes.pop()) {
                    attrs.insert(attr.type(), attr.value());
                }
                a.setAttributes(attrs);
            }

            rv << a;
        }
    }

    return rv;
}

Protocol::FetchTagsResponse HandlerHelper::fetchTagsResponse(const Tag &tag,
                                                             bool withRID,
                                                             Connection *connection)
{
    Protocol::FetchTagsResponse response(tag.id());
    response.setType(tag.tagType().name().toUtf8());
    response.setParentId(tag.parentId());
    response.setGid(tag.gid().toUtf8());

    if (withRID && connection) {
        // Fail silently if retrieving tag RID is not allowed in current context
        if (!connection->context()->resource().isValid()) {
            return response;
        }

        QueryBuilder qb(TagRemoteIdResourceRelation::tableName());
        qb.addColumn(TagRemoteIdResourceRelation::remoteIdColumn());
        qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdColumn(),
                             Query::Equals,
                             connection->context()->resource().id());
        qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(),
                             Query::Equals,
                             tag.id());
        if (!qb.exec()) {
            throw HandlerException("Unable to query Tag Remote ID");
        }
        QSqlQuery query = qb.query();
        // No RID for this tag
        if (!query.next()) {
            return response;
        }
        response.setRemoteId(Utils::variantToByteArray(query.value(0)));
    }

    return response;
}

Protocol::FetchRelationsResponse HandlerHelper::fetchRelationsResponse(const Relation &relation)
{
    return Protocol::FetchRelationsResponse(relation.leftId(),
                                            relation.left().mimeType().name().toUtf8(),
                                            relation.rightId(),
                                            relation.right().mimeType().name().toUtf8(),
                                            relation.relationType().name().toUtf8());
}


Flag::List HandlerHelper::resolveFlags(const QSet<QByteArray> &flagNames)
{
    Flag::List flagList;
    flagList.reserve(flagNames.size());
    Q_FOREACH (const QByteArray &flagName, flagNames) {
        const Flag flag = Flag::retrieveByNameOrCreate(QString::fromUtf8(flagName));
        if (!flag.isValid()) {
            throw HandlerException("Unable to create flag");
        }
        flagList.append(flag);
    }
    return flagList;
}

Tag::List HandlerHelper::resolveTagsByUID(const ImapSet &tags)
{
    if (tags.isEmpty()) {
        return Tag::List();
    }
    SelectQueryBuilder<Tag> qb;
    QueryHelper::setToQuery(tags, Tag::idFullColumnName(), qb);
    if (!qb.exec()) {
        throw HandlerException("Unable to resolve tags");
    }
    const Tag::List result = qb.result();
    if (result.isEmpty()) {
        throw HandlerException("No tags found");
    }
    return result;
}

Tag::List HandlerHelper::resolveTagsByGID(const QStringList &tagsGIDs)
{
    Tag::List tagList;
    if (tagsGIDs.isEmpty()) {
        return tagList;
    }

    for (const QString &tagGID : tagsGIDs) {
        Tag::List tags = Tag::retrieveFiltered(Tag::gidColumn(), tagGID);
        Tag tag;
        if (tags.isEmpty()) {
            tag.setGid(tagGID);
            tag.setParentId(0);

            const TagType type = TagType::retrieveByNameOrCreate(QStringLiteral("PLAIN"));
            if (!type.isValid()) {
                throw HandlerException("Unable to create tag type");
            }
            tag.setTagType(type);
            if (!tag.insert()) {
                throw HandlerException("Unable to create tag");
            }
        } else if (tags.count() == 1) {
            tag = tags[0];
        } else {
            // Should not happen
            throw HandlerException("Tag GID is not unique");
        }

        tagList.append(tag);
    }

    return tagList;
}

Tag::List HandlerHelper::resolveTagsByRID(const QStringList &tagsRIDs, CommandContext *context)
{
    Tag::List tags;
    if (tagsRIDs.isEmpty()) {
        return tags;
    }

    if (!context->resource().isValid()) {
        throw HandlerException("Tags can be resolved by their RID only in resource context");
    }

    tags.reserve(tagsRIDs.size());
    for (const QString &tagRID : tagsRIDs) {
        SelectQueryBuilder<Tag> qb;
        Query::Condition cond;
        cond.addColumnCondition(Tag::idFullColumnName(), Query::Equals, TagRemoteIdResourceRelation::tagIdFullColumnName());
        cond.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, context->resource().id());
        qb.addJoin(QueryBuilder::LeftJoin, TagRemoteIdResourceRelation::tableName(), cond);
        qb.addValueCondition(TagRemoteIdResourceRelation::remoteIdFullColumnName(), Query::Equals, tagRID);
        if (!qb.exec()) {
            throw HandlerException("Unable to resolve tags");
        }

        Tag tag;
        Tag::List results = qb.result();
        if (results.isEmpty()) {
            // If the tag does not exist, we create a new one with GID matching RID
            Tag::List tags = resolveTagsByGID(QStringList() << tagRID);
            if (tags.count() != 1) {
                throw HandlerException("Unable to resolve tag");
            }
            tag = tags[0];
            TagRemoteIdResourceRelation rel;
            rel.setRemoteId(tagRID);
            rel.setTagId(tag.id());
            rel.setResourceId(context->resource().id());
            if (!rel.insert()) {
                throw HandlerException("Unable to create tag");
            }
        } else if (results.count() == 1) {
            tag = results[0];
        } else {
            throw HandlerException("Tag RID is not unique within this resource context");
        }

        tags.append(tag);
    }

    return tags;
}

Collection HandlerHelper::collectionFromScope(const Scope &scope, Connection *connection)
{
    if (scope.scope() == Scope::Invalid || scope.scope() == Scope::Gid) {
        throw HandlerException("Invalid collection scope");
    }

    SelectQueryBuilder<Collection> qb;
    CollectionQueryHelper::scopeToQuery(scope, connection, qb);
    if (!qb.exec()) {
        throw HandlerException("Failed to execute SQL query");
    }

    const Collection::List c = qb.result();
    if (c.count() == 0) {
        return Collection();
    } else if (c.count() == 1) {
        return c.at(0);
    } else  {
        throw HandlerException("Query returned more than one reslut");
    }
}

Tag::List HandlerHelper::tagsFromScope(const Scope &scope, Connection* connection)
{
    if (scope.scope() == Scope::Invalid || scope.scope() == Scope::HierarchicalRid) {
        throw HandlerException("Invalid tag scope");
    }

    if (scope.scope() == Scope::Uid) {
        return resolveTagsByUID(scope.uidSet());
    } else if (scope.scope() == Scope::Gid) {
        return resolveTagsByGID(scope.gidSet());
    } else if (scope.scope() == Scope::Rid) {
        return resolveTagsByRID(scope.ridSet(), connection->context());
    }

    Q_ASSERT(false);
    return Tag::List();
}
