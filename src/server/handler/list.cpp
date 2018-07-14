/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "list.h"
#include "akonadiserver_debug.h"


#include "connection.h"
#include "handlerhelper.h"
#include "collectionreferencemanager.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

template <typename T>
static bool intersect(const QVector<typename T::Id> &l1, const QVector<T> &l2)
{
    for (const T &e2 : l2) {
        if (l1.contains(e2.id())) {
            return true;
        }
    }
    return false;
}

QStack<Collection> List::ancestorsForCollection(const Collection &col)
{
    if (mAncestorDepth <= 0) {
        return QStack<Collection>();
    }
    QStack<Collection> ancestors;
    Collection parent = col;
    for (int i = 0; i < mAncestorDepth; ++i) {
        if (parent.parentId() == 0) {
            break;
        }
        if (mAncestors.contains(parent.parentId())) {
            parent = mAncestors.value(parent.parentId());
        } else {
            parent = mCollections.value(parent.parentId());
        }
        if (!parent.isValid()) {
            qCWarning(AKONADISERVER_LOG) << col.id();
            throw HandlerException("Found invalid parent in ancestors");
        }
        ancestors.prepend(parent);
    }
    return ancestors;
}

CollectionAttribute::List List::getAttributes(const Collection &col, const QSet<QByteArray> &filter)
{
    CollectionAttribute::List attributes;
    auto it = mCollectionAttributes.find(col.id());
    while (it != mCollectionAttributes.end() && it.key() == col.id()) {
        if (filter.isEmpty() || filter.contains(it.value().type())) {
            attributes << it.value();
        }
        ++it;
    }

    // We need the reference and enabled status, to not need to request the server mutliple times.
    // Mostly interessting for ancestors for i.e. updates provided by the monitor.
    const bool isReferenced = connection()->collectionReferenceManager()->isReferenced(col.id(), connection()->sessionId());
    if (isReferenced) {
        CollectionAttribute attr;
        attr.setType(AKONADI_PARAM_REFERENCED);
        attr.setValue("TRUE");
        attributes << attr;
    }
    {
        CollectionAttribute attr;
        attr.setType(AKONADI_PARAM_ENABLED);
        attr.setValue(col.enabled() ? "TRUE" : "FALSE");
        attributes << attr;
    }

    return attributes;
}

void List::listCollection(const Collection &root, const QStack<Collection> &ancestors,
                          const QStringList &mimeTypes,
                          const CollectionAttribute::List &attributes)
{
    const bool isReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(root.id(), connection()->sessionId());
    //We always expose referenced collections to the resource as referenced (although it's a different session)
    //Otherwise syncing wouldn't work.
    const bool resourceIsSynchronizing = root.referenced() && mCollectionsToSynchronize && connection()->context()->resource().isValid();

    QStack<CollectionAttribute::List> ancestorAttributes;
    //backwards compatibility, collectionToByteArray will automatically fall-back to id + remoteid
    if (!mAncestorAttributes.isEmpty()) {
        ancestorAttributes.reserve(ancestors.size());
        for (const Collection &col : ancestors) {
            ancestorAttributes.push(getAttributes(col, mAncestorAttributes));
        }
    }

    // write out collection details
    Collection dummy = root;
    DataStore *db = connection()->storageBackend();
    db->activeCachePolicy(dummy);

    sendResponse(HandlerHelper::fetchCollectionsResponse(dummy, attributes, mIncludeStatistics,
                 mAncestorDepth, ancestors,
                 ancestorAttributes,
                 isReferencedFromSession || resourceIsSynchronizing,
                 mimeTypes));
}

static Query::Condition filterCondition(const QString &column)
{
    Query::Condition orCondition(Query::Or);
    orCondition.addValueCondition(column, Query::Equals, (int)Collection::True);
    Query::Condition andCondition(Query::And);
    andCondition.addValueCondition(column, Query::Equals, (int)Collection::Undefined);
    andCondition.addValueCondition(Collection::enabledFullColumnName(), Query::Equals, true);
    orCondition.addCondition(andCondition);
    orCondition.addValueCondition(Collection::referencedFullColumnName(), Query::Equals, true);
    return orCondition;
}

bool List::checkFilterCondition(const Collection &col) const
{
    //Don't include the collection when only looking for enabled collections
    if (mEnabledCollections && !col.enabled()) {
        return false;
    }
    //Don't include the collection when only looking for collections to display/index/sync
    if (mCollectionsToDisplay &&
            (((col.displayPref() == Collection::Undefined) && !col.enabled()) ||
             (col.displayPref() == Collection::False))) {
        return false;
    }
    if (mCollectionsToIndex &&
            (((col.indexPref() == Collection::Undefined) && !col.enabled()) ||
             (col.indexPref() == Collection::False))) {
        return false;
    }
    //Single collection sync will still work since that is using a base fetch
    if (mCollectionsToSynchronize &&
            (((col.syncPref() == Collection::Undefined) && !col.enabled()) ||
             (col.syncPref() == Collection::False))) {
        return false;
    }
    return true;
}

static QSqlQuery getAttributeQuery(const QVariantList &ids, const QSet<QByteArray> &requestedAttributes)
{
    QueryBuilder qb(CollectionAttribute::tableName());

    qb.addValueCondition(CollectionAttribute::collectionIdFullColumnName(), Query::In, ids);

    qb.addColumn(CollectionAttribute::collectionIdFullColumnName());
    qb.addColumn(CollectionAttribute::typeFullColumnName());
    qb.addColumn(CollectionAttribute::valueFullColumnName());

    if (!requestedAttributes.isEmpty()) {
        QVariantList attributes;
        attributes.reserve(requestedAttributes.size());
        for (const QByteArray &type : requestedAttributes) {
            attributes << type;
        }
        qb.addValueCondition(CollectionAttribute::typeFullColumnName(), Query::In, attributes);
    }

    qb.addSortColumn(CollectionAttribute::collectionIdFullColumnName(), Query::Ascending);

    if (!qb.exec()) {
        throw HandlerException("Unable to retrieve attributes for listing");
    }
    return qb.query();
}

void List::retrieveAttributes(const QVariantList &collectionIds)
{
    //We are querying for the attributes in batches because something can't handle WHERE IN queries with sets larger than 999
    int start = 0;
    const int size = 999;
    while (start < collectionIds.size()) {
        const QVariantList ids = collectionIds.mid(start, size);
        QSqlQuery attributeQuery = getAttributeQuery(ids, mAncestorAttributes);
        while (attributeQuery.next()) {
            CollectionAttribute attr;
            attr.setType(attributeQuery.value(1).toByteArray());
            attr.setValue(attributeQuery.value(2).toByteArray());
            // qCDebug(AKONADISERVER_LOG) << "found attribute " << attr.type() << attr.value();
            mCollectionAttributes.insert(attributeQuery.value(0).toLongLong(), attr);
        }
        start += size;
    }
}

static QSqlQuery getMimeTypeQuery(const QVariantList &ids)
{
    QueryBuilder qb(CollectionMimeTypeRelation::tableName());

    qb.addJoin(QueryBuilder::LeftJoin, MimeType::tableName(), MimeType::idFullColumnName(), CollectionMimeTypeRelation::rightFullColumnName());
    qb.addValueCondition(CollectionMimeTypeRelation::leftFullColumnName(), Query::In, ids);

    qb.addColumn(CollectionMimeTypeRelation::leftFullColumnName());
    qb.addColumn(CollectionMimeTypeRelation::rightFullColumnName());
    qb.addColumn(MimeType::nameFullColumnName());
    qb.addSortColumn(CollectionMimeTypeRelation::leftFullColumnName(), Query::Ascending);

    if (!qb.exec()) {
        throw HandlerException("Unable to retrieve mimetypes for listing");
    }
    return qb.query();
}

void List::retrieveCollections(const Collection &topParent, int depth)
{
    /*
     * Retrieval of collections:
     * The aim is to reduce the amount of queries as much as possible, as this has the largest performance impact for large queries.
     * * First all collections that match the given criteria are queried
     * * We then filter the false positives:
     * ** all collections out that are not part of the tree we asked for are filtered
     * ** all collections that are referenced but not by this session or by the owning resource are filtered
     * * Finally we complete the tree by adding missing collections
     *
     * Mimetypes and attributes are also retrieved in single queries to avoid spawning two queries per collection (the N+1 problem).
     * Note that we're not querying attributes and mimetypes for the collections that are only included to complete the tree,
     * this results in no items being queried for those collections.
     */

    const qint64 parentId = topParent.isValid() ? topParent.id() : 0;
    {
        SelectQueryBuilder<Collection> qb;

        if (depth == 0) {
            qb.addValueCondition(Collection::idFullColumnName(), Query::Equals, parentId);
        } else if (depth == 1) {
            if (topParent.isValid()) {
                qb.addValueCondition(Collection::parentIdFullColumnName(), Query::Equals, parentId);
            } else {
                qb.addValueCondition(Collection::parentIdFullColumnName(), Query::Is, QVariant());
            }
        } else {
            if (topParent.isValid()) {
                qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, topParent.resourceId());
            } else {
                // Gimme gimme gimme...everything!
            }
        }

        //Base listings should succeed always
        if (depth != 0) {
            if (mCollectionsToSynchronize) {
                qb.addCondition(filterCondition(Collection::syncPrefFullColumnName()));
            } else if (mCollectionsToDisplay) {
                qCDebug(AKONADISERVER_LOG) << "only display";
                qb.addCondition(filterCondition(Collection::displayPrefFullColumnName()));
            } else if (mCollectionsToIndex) {
                qb.addCondition(filterCondition(Collection::indexPrefFullColumnName()));
            } else if (mEnabledCollections) {
                Query::Condition orCondition(Query::Or);
                orCondition.addValueCondition(Collection::enabledFullColumnName(), Query::Equals, true);
                orCondition.addValueCondition(Collection::referencedFullColumnName(), Query::Equals, true);
                qb.addCondition(orCondition);
            }
            if (mResource.isValid()) {
                qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, mResource.id());
            }

            if (!mMimeTypes.isEmpty()) {
                qb.addJoin(QueryBuilder::LeftJoin, CollectionMimeTypeRelation::tableName(), CollectionMimeTypeRelation::leftColumn(), Collection::idFullColumnName());
                QVariantList mimeTypeFilter;
                mimeTypeFilter.reserve(mMimeTypes.size());
                for (MimeType::Id mtId : qAsConst(mMimeTypes)) {
                    mimeTypeFilter << mtId;
                }
                qb.addValueCondition(CollectionMimeTypeRelation::rightColumn(), Query::In, mimeTypeFilter);
                qb.addGroupColumn(Collection::idFullColumnName());
            }
        }

        if (!qb.exec()) {
            throw HandlerException("Unable to retrieve collection for listing");
        }
        Q_FOREACH (const Collection &col, qb.result()) {
            mCollections.insert(col.id(), col);
        }
    }

    //Post filtering that we couldn't do as part of the sql query
    if (depth > 0) {
        auto it = mCollections.begin();
        while (it != mCollections.end()) {

            if (topParent.isValid()) {
                //Check that each collection is linked to the root collection
                bool foundParent = false;
                //We iterate over parents to link it to topParent if possible
                Collection::Id id = it->parentId();
                while (id > 0) {
                    if (id == parentId) {
                        foundParent = true;
                        break;
                    }
                    Collection col = mCollections.value(id);
                    if (!col.isValid()) {
                        col = Collection::retrieveById(id);
                    }
                    id = col.parentId();
                }
                if (!foundParent) {
                    it = mCollections.erase(it);
                    continue;
                }
            }

            //If we matched referenced collections we need to ensure the collection was referenced from this session
            const bool isReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(it->id(), connection()->sessionId());
            //The collection is referenced, but not from this session. We need to reevaluate the filter condition
            if (it->referenced() && !isReferencedFromSession) {
                //Don't include the collection when only looking for enabled collections.
                //However, a referenced collection should be still synchronized by the resource, so we exclude this case.
                if (!checkFilterCondition(*it) && !(mCollectionsToSynchronize && connection()->context()->resource().isValid())) {
                    it = mCollections.erase(it);
                    continue;
                }
            }

            ++it;
        }
    }

    QVariantList mimeTypeIds;
    QVariantList attributeIds;
    QVariantList ancestorIds;
    const int collectionSize{mCollections.size()};
    mimeTypeIds.reserve(collectionSize);
    attributeIds.reserve(collectionSize);
    //We'd only require the non-leaf collections, but we don't know which those are, so we take all.
    ancestorIds.reserve(collectionSize);
    for (auto it = mCollections.cbegin(), end = mCollections.cend(); it != end; ++it) {
        mimeTypeIds << it.key();
        attributeIds << it.key();
        ancestorIds << it.key();
    }

    if (mAncestorDepth > 0 && topParent.isValid()) {
        //unless depth is 0 the base collection is not part of the listing
        mAncestors.insert(topParent.id(), topParent);
        ancestorIds << topParent.id();
        //We need to retrieve additional ancestors to what we already have in the tree
        Collection parent = topParent;
        for (int i = 0; i < mAncestorDepth; ++i) {
            if (parent.parentId() == 0) {
                break;
            }
            parent = parent.parent();
            mAncestors.insert(parent.id(), parent);
            //We also require the attributes
            ancestorIds << parent.id();
        }
    }

    QSet<qint64> missingCollections;
    if (depth > 0) {
        for (const Collection &col : qAsConst(mCollections)) {
            if (col.parentId() != parentId && !mCollections.contains(col.parentId())) {
                missingCollections.insert(col.parentId());
            }
        }
    }

    /*
    QSet<qint64> knownIds;
    for (const Collection &col : mCollections) {
        knownIds.insert(col.id());
    }
    qCDebug(AKONADISERVER_LOG) << "HAS:" << knownIds;
    qCDebug(AKONADISERVER_LOG) << "MISSING:" << missingCollections;
    */

    //Fetch missing collections that are part of the tree
    while (!missingCollections.isEmpty()) {
        SelectQueryBuilder<Collection> qb;
        QVariantList ids;
        ids.reserve(missingCollections.size());
        for (qint64 id : qAsConst(missingCollections)) {
            ids << id;
        }
        qb.addValueCondition(Collection::idFullColumnName(), Query::In, ids);
        if (!qb.exec()) {
            throw HandlerException("Unable to retrieve collections for listing");
        }

        missingCollections.clear();
        Q_FOREACH (const Collection &missingCol, qb.result()) {
            mCollections.insert(missingCol.id(), missingCol);
            ancestorIds << missingCol.id();
            attributeIds << missingCol.id();
            mimeTypeIds << missingCol.id();
            //We have to do another round if the parents parent is missing
            if (missingCol.parentId() != parentId && !mCollections.contains(missingCol.parentId())) {
                missingCollections.insert(missingCol.parentId());
            }
        }
    }

    //Since we don't know when we'll need the ancestor attributes, we have to fetch them all together.
    //The alternative would be to query for each collection which would reintroduce the N+1 query performance problem.
    if (!mAncestorAttributes.isEmpty()) {
        retrieveAttributes(ancestorIds);
    }

    //We are querying in batches because something can't handle WHERE IN queries with sets larger than 999
    const int querySizeLimit = 999;
    int mimetypeQueryStart = 0;
    int attributeQueryStart = 0;
    QSqlQuery mimeTypeQuery(DataStore::self()->database());
    QSqlQuery attributeQuery(DataStore::self()->database());
    auto it = mCollections.begin();
    while (it != mCollections.end()) {
        const Collection col = it.value();
        // qCDebug(AKONADISERVER_LOG) << "col " << col.id();

        QStringList mimeTypes;
        {
            //Get new query if necessary
            if (!mimeTypeQuery.isValid() && mimetypeQueryStart < mimeTypeIds.size()) {
                const QVariantList ids = mimeTypeIds.mid(mimetypeQueryStart, querySizeLimit);
                mimetypeQueryStart += querySizeLimit;
                mimeTypeQuery = getMimeTypeQuery(ids);
                mimeTypeQuery.next(); //place at first record
            }

            // qCDebug(AKONADISERVER_LOG) << mimeTypeQuery.isValid() << mimeTypeQuery.value(0).toLongLong();
            while (mimeTypeQuery.isValid() && mimeTypeQuery.value(0).toLongLong() < col.id()) {
                qCDebug(AKONADISERVER_LOG) << "skipped: " << mimeTypeQuery.value(0).toLongLong() << mimeTypeQuery.value(2).toString();
                if (!mimeTypeQuery.next()) {
                    break;
                }
            }
            //Advance query while a mimetype for this collection is returned
            while (mimeTypeQuery.isValid() && mimeTypeQuery.value(0).toLongLong() == col.id()) {
                mimeTypes << mimeTypeQuery.value(2).toString();
                if (!mimeTypeQuery.next()) {
                    break;
                }
            }
        }

        CollectionAttribute::List attributes;
        {
            //Get new query if necessary
            if (!attributeQuery.isValid() && attributeQueryStart < attributeIds.size()) {
                const QVariantList ids = attributeIds.mid(attributeQueryStart, querySizeLimit);
                attributeQueryStart += querySizeLimit;
                attributeQuery = getAttributeQuery(ids, QSet<QByteArray>());
                attributeQuery.next(); //place at first record
            }

            // qCDebug(AKONADISERVER_LOG) << attributeQuery.isValid() << attributeQuery.value(0).toLongLong();
            while (attributeQuery.isValid() && attributeQuery.value(0).toLongLong() < col.id()) {
                qCDebug(AKONADISERVER_LOG) << "skipped: " << attributeQuery.value(0).toLongLong() << attributeQuery.value(1).toByteArray();
                if (!attributeQuery.next()) {
                    break;
                }
            }
            //Advance query while a mimetype for this collection is returned
            while (attributeQuery.isValid() && attributeQuery.value(0).toLongLong() == col.id()) {
                CollectionAttribute attr;
                attr.setType(attributeQuery.value(1).toByteArray());
                attr.setValue(attributeQuery.value(2).toByteArray());
                attributes << attr;

                if (!attributeQuery.next()) {
                    break;
                }
            }
        }

        listCollection(col, ancestorsForCollection(col), mimeTypes, attributes);
        it++;
    }
}

bool List::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::FetchCollectionsCommand>(m_command);

    if (!cmd.resource().isEmpty()) {
        mResource = Resource::retrieveByName(cmd.resource());
        if (!mResource.isValid()) {
            return failureResponse("Unknown resource");
        }
    }
    const QStringList lstMimeTypes = cmd.mimeTypes();
    for (const QString &mtName : lstMimeTypes) {
        const MimeType mt = MimeType::retrieveByNameOrCreate(mtName);
        if (!mt.isValid()) {
            return failureResponse("Failed to create mimetype record");
        }
        mMimeTypes.append(mt.id());
    }

    mEnabledCollections = cmd.enabled();
    mCollectionsToSynchronize = cmd.syncPref();
    mCollectionsToDisplay = cmd.displayPref();
    mCollectionsToIndex = cmd.indexPref();
    mIncludeStatistics = cmd.fetchStats();

    int depth = 0;
    switch (cmd.depth()) {
    case Protocol::FetchCollectionsCommand::BaseCollection:
        depth = 0;
        break;
    case Protocol::FetchCollectionsCommand::ParentCollection:
        depth = 1;
        break;
    case Protocol::FetchCollectionsCommand::AllCollections:
        depth = INT_MAX;
        break;
    }

    switch (cmd.ancestorsDepth()) {
    case Protocol::Ancestor::NoAncestor:
        mAncestorDepth = 0;
        break;
    case Protocol::Ancestor::ParentAncestor:
        mAncestorDepth = 1;
        break;
    case Protocol::Ancestor::AllAncestors:
        mAncestorDepth = INT_MAX;
        break;
    }
    mAncestorAttributes = cmd.ancestorsAttributes();

    Scope scope = cmd.collections();
    if (!scope.isEmpty()) { // not root
        Collection col;
        if (scope.scope() == Scope::Uid) {
            col = Collection::retrieveById(scope.uid());
        } else if (scope.scope() == Scope::Rid) {
            SelectQueryBuilder<Collection> qb;
            qb.addValueCondition(Collection::remoteIdFullColumnName(), Query::Equals, scope.rid());
            qb.addJoin(QueryBuilder::InnerJoin, Resource::tableName(),
                       Collection::resourceIdFullColumnName(), Resource::idFullColumnName());
            if (mCollectionsToSynchronize) {
                qb.addCondition(filterCondition(Collection::syncPrefFullColumnName()));
            } else if (mCollectionsToDisplay) {
                qb.addCondition(filterCondition(Collection::displayPrefFullColumnName()));
            } else if (mCollectionsToIndex) {
                qb.addCondition(filterCondition(Collection::indexPrefFullColumnName()));
            }
            if (mResource.isValid()) {
                qb.addValueCondition(Resource::idFullColumnName(), Query::Equals, mResource.id());
            } else if (connection()->context()->resource().isValid()) {
                qb.addValueCondition(Resource::idFullColumnName(), Query::Equals, connection()->context()->resource().id());
            } else {
                return failureResponse("Cannot retrieve collection based on remote identifier without a resource context");
            }
            if (!qb.exec()) {
                return failureResponse("Unable to retrieve collection for listing");
            }
            Collection::List results = qb.result();
            if (results.count() != 1) {
                return failureResponse(QString::number(results.count()) + QStringLiteral(" collections found"));
            }
            col = results.first();
        } else if (scope.scope() == Scope::HierarchicalRid) {
            if (!connection()->context()->resource().isValid()) {
                return failureResponse("Cannot retrieve collection based on hierarchical remote identifier without a resource context");
            }
            col = CollectionQueryHelper::resolveHierarchicalRID(scope.hridChain(), connection()->context()->resource().id());
        } else {
            return failureResponse("Unexpected error");
        }

        if (!col.isValid()) {
            return failureResponse("Collection does not exist");
        }

        retrieveCollections(col, depth);
    } else { //Root folder listing
        if (depth != 0) {
            retrieveCollections(Collection(), depth);
        }
    }

    return successResponse<Protocol::FetchCollectionsResponse>();
}
