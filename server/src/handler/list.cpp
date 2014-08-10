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

#include <QtCore/QDebug>

#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/selectquerybuilder.h"

#include "connection.h"
#include "response.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "collectionreferencemanager.h"

#include <libs/protocol_p.h>
#include <storage/collectionqueryhelper.h>

using namespace Akonadi::Server;

template <typename T>
static bool intersect(const QVector<typename T::Id> &l1, const QVector<T> &l2)
{
    Q_FOREACH (const T &e2, l2) {
        if (l1.contains(e2.id())) {
            return true;
        }
    }
    return false;
}

List::List(Scope::SelectionScope scope, bool onlySubscribed)
    : Handler()
    , mScope(scope)
    , mAncestorDepth(0)
    , mOnlySubscribed(onlySubscribed)
    , mIncludeStatistics(false)
    , mEnabledCollections(false)
    , mCollectionsToDisplay(false)
    , mCollectionsToSynchronize(false)
    , mCollectionsToIndex(false)
{
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
        parent = parent.parent();
        ancestors.prepend(parent);
    }
    return ancestors;
}

void List::listCollection(const Collection &root, const QStack<Collection> &ancestors)
{
    //We exclude the mimetypes of the collection if this would normally be hidden but is required to complete the tree
    //TODO: should the enabled state also contribute to hidden?
    const bool hidden = (!mMimeTypes.isEmpty() && !intersect(mMimeTypes, root.mimeTypes()));
    const bool isReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(root.id(), connection()->sessionId());
    //We always expose referenced collections to the resource as referenced (although it's a different session)
    //Otherwise syncing wouldn't work.
    const bool resourceIsSynchronizing = root.referenced() && mCollectionsToSynchronize && connection()->context()->resource().isValid();

    // write out collection details
    Collection dummy = root;
    DataStore *db = connection()->storageBackend();
    db->activeCachePolicy(dummy);
    const QByteArray b = HandlerHelper::collectionToByteArray(dummy, hidden, mIncludeStatistics, mAncestorDepth, ancestors, isReferencedFromSession || resourceIsSynchronizing, mAncestorAttributes);

    Response response;
    response.setUntagged();
    response.setString(b);
    Q_EMIT responseAvailable(response);
}

static Query::Condition filterCondition(const QString &column)
{
    Query::Condition orCondition(Query::Or);
    orCondition.addValueCondition(column, Query::Equals, Akonadi::Server::Tristate::True);
    Query::Condition andCondition(Query::And);
    andCondition.addValueCondition(column, Query::Equals, Akonadi::Server::Tristate::Undefined);
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
            (((col.displayPref() == Tristate::Undefined) && !col.enabled()) ||
            (col.displayPref() == Tristate::False))) {
        return false;
    }
    if (mCollectionsToIndex &&
            (((col.indexPref() == Tristate::Undefined) && !col.enabled()) ||
            (col.indexPref() == Tristate::False))) {
        return false;
    }
    //Single collection sync will still work since that is using a base fetch
    if (mCollectionsToSynchronize &&
            (((col.syncPref() == Tristate::Undefined) && !col.enabled()) ||
            (col.syncPref() == Tristate::False))) {
        return false;
    }
    return true;
}

bool List::checkChildrenForMimeTypes(const QHash<qint64, Collection> &collectionsMap,
                                     const QHash<qint64, qint64> &parentLookup,
                                     const Collection &col)
{
    const QList<qint64> children = parentLookup.values(col.id());
    Q_FOREACH (qint64 childId, children) {
        if (!collectionsMap.contains(childId)) {
            continue;
        }

        const Collection &child = collectionsMap[childId];
        if (!intersect(mMimeTypes, child.mimeTypes())) {
            if (checkChildrenForMimeTypes(collectionsMap, parentLookup, child)) {
                return true;
            }
        }
    }

    return false;
}

Collection::List List::retrieveChildren(const Collection &topParent, int depth)
{
    QHash<qint64 /*id*/, Collection> collections;
    QMultiHash<qint64 /* parent */, qint64 /* children */> parentLookup;

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

        if (mCollectionsToSynchronize) {
            qb.addCondition(filterCondition(Collection::syncPrefFullColumnName()));
        } else if (mCollectionsToDisplay) {
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

        if (!qb.exec()) {
            throw HandlerException("Unable to retrieve collection for listing");
        }
        Q_FOREACH (const Collection &col, qb.result()) {
            collections.insert(col.id(), col);
            parentLookup.insertMulti(col.parentId(), col.id());
        }
    }

    //Post filtering that we couldn't do as part of the sql query
    if (topParent.isValid() || !mMimeTypes.isEmpty()) {
        auto it = collections.begin();
        while (it != collections.end()) {
            //Filter by mimetype
            if (!mMimeTypes.isEmpty()) {
                // filter if this node isn't needed by it's children
                const bool hidden = !intersect(mMimeTypes, it->mimeTypes());
                const bool hasChildCollections = checkChildrenForMimeTypes(collections, parentLookup, (*it));
                if (hidden && !hasChildCollections) {
                    Q_FOREACH (qint64 id, parentLookup.keys(it->id())) {
                        parentLookup.remove(id, it->id());
                    }
                    parentLookup.remove(it->id());
                    it = collections.erase(it);
                    continue;
                }
            }

            //Check that each collection is linked to the root collection
            if (topParent.isValid()) {
                bool foundParent = false;
                //We iterate over parents to link it to topParent if possible
                Collection::Id id = it->parentId();
                while (id > 0) {
                    if (id == parentId) {
                        foundParent = true;
                        break;
                    }
                    const qint64 pId = collections.value(id).parentId();
                    if (pId >= 0) {
                        id = pId;
                    } else {
                        //parentMap doesn't necessarily contain all nodes of the tree
                        id = Collection::retrieveById(id).parentId();
                    }
                }
                if (!foundParent) {
                    Q_FOREACH (qint64 id, parentLookup.keys(it->id())) {
                        parentLookup.remove(id, it->id());
                    }
                    parentLookup.remove(it->id());
                    it = collections.erase(it);
                    continue;
                }
            }

            ++it;
        }
    }

    const bool listFilterEnabled = mEnabledCollections || mCollectionsToIndex || mCollectionsToDisplay || mCollectionsToSynchronize;

    //If we matched referenced collecions we need to ensure the collection was referenced from this session
    if (listFilterEnabled) {
        auto it = collections.begin();
        while (it != collections.end()) {
            const bool isReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(it->id(), connection()->sessionId());
            //The collection is referenced, but not from this session. We need to reevaluate the filter condition
            if (it->referenced() && !isReferencedFromSession) {
                //Don't include the collection when only looking for enabled collections.
                //However, a referenced collection should be still synchronized by the resource, so we exclude this case.
                if (!checkFilterCondition(*it) && !(mCollectionsToSynchronize && connection()->context()->resource().isValid())) {
                    Q_FOREACH (qint64 id, parentLookup.keys(it->id())) {
                        parentLookup.remove(id, it->id());
                    }
                    parentLookup.remove(it->id());
                    it = collections.erase(it);
                    continue;
                }
            }
            it++;
        }
    }

    QSet<qint64> missingCollections;
    Q_FOREACH (const Collection &col, collections) {
        if (col.parentId() != parentId && !collections.contains(col.parentId())) {
            missingCollections.insert(col.parentId());
        }
    }

    // qDebug() << "HAS:" << knownIds;
    // qDebug() << "MISSING:" << missingCollections;

    //Fetch missing collections that are part of the tree
    if (!missingCollections.isEmpty()) {
        SelectQueryBuilder<Collection> qb;
        QVariantList ids;
        ids.reserve(missingCollections.size());
        Q_FOREACH (qint64 id, missingCollections) {
            ids << id;
        }
        qb.addValueCondition(Collection::idFullColumnName(), Query::In, ids);
        if (!qb.exec()) {
            throw HandlerException("Unable to retrieve collections for listing");
        }

        Q_FOREACH (const Collection &missingCol, qb.result()) {
            collections.insert(missingCol.id(), missingCol);
        }
    }
    return collections.values().toVector();
}

bool List::parseStream()
{
    qint64 baseCollection = -1;
    QString rid;
    if (mScope.scope() == Scope::None || mScope.scope() == Scope::Uid) {
        bool ok = false;
        baseCollection = m_streamParser->readNumber(&ok);
        if (!ok) {
            return failureResponse("Invalid base collection");
        }
    } else if (mScope.scope() == Scope::Rid) {
        rid = m_streamParser->readUtf8String();
        if (rid.isEmpty()) {
            throw HandlerException("No remote identifier specified");
        }
    } else if (mScope.scope() == Scope::HierarchicalRid) {
        mScope.parseScope(m_streamParser);
    } else {
        throw HandlerException("WTF");
    }

    const int depth = HandlerHelper::parseDepth(m_streamParser->readString());

    m_streamParser->beginList();
    while (!m_streamParser->atListEnd()) {
        const QByteArray filter = m_streamParser->readString();
        if (filter == AKONADI_PARAM_RESOURCE) {
            mResource = Resource::retrieveByName(m_streamParser->readUtf8String());
            if (!mResource.isValid()) {
                return failureResponse("Unknown resource");
            }
        } else if (filter == AKONADI_PARAM_MIMETYPE) {
            m_streamParser->beginList();
            while (!m_streamParser->atListEnd()) {
                const QString mtName = m_streamParser->readUtf8String();
                const MimeType mt = MimeType::retrieveByName(mtName);
                if (mt.isValid()) {
                    mMimeTypes.append(mt.id());
                } else {
                    MimeType mt(mtName);
                    if (!mt.insert()) {
                        throw HandlerException("Failed to create mimetype record");
                    }
                    mMimeTypes.append(mt.id());
                }
            }
        } else if (filter == AKONADI_PARAM_ENABLED) {
            mEnabledCollections = true;
        } else if (filter == AKONADI_PARAM_SYNC) {
            mCollectionsToSynchronize = true;
        } else if (filter == AKONADI_PARAM_DISPLAY) {
            mCollectionsToDisplay = true;
        } else if (filter == AKONADI_PARAM_INDEX) {
            mCollectionsToIndex = true;
        }
    }

    //For backwards compatibilty with the subscription mechanism
    mEnabledCollections = mEnabledCollections || mOnlySubscribed;

    if (m_streamParser->hasList()) {   // We got extra options
        m_streamParser->beginList();
        while (!m_streamParser->atListEnd()) {
            const QByteArray option = m_streamParser->readString();
            if (option == AKONADI_PARAM_STATISTICS) {
                if (m_streamParser->readString() == "true") {
                    mIncludeStatistics = true;
                }
            }
            if (option == AKONADI_PARAM_ANCESTORS) {
                const QByteArray argument = m_streamParser->readString();
                mAncestorDepth = HandlerHelper::parseDepth(argument);
            }
            if (option == AKONADI_PARAM_ANCESTORATTRIBUTE) {
                const QByteArray argument = m_streamParser->readString();
                mAncestorAttributes << argument;
            }
        }
    }

    Collection::List collections;
    QStack<Collection> ancestors;

    if (baseCollection != 0) { // not root
        Collection col;
        if (mScope.scope() == Scope::None || mScope.scope() == Scope::Uid) {
            col = Collection::retrieveById(baseCollection);
        } else if (mScope.scope() == Scope::Rid) {
            SelectQueryBuilder<Collection> qb;
            qb.addValueCondition(Collection::remoteIdFullColumnName(), Query::Equals, rid);
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
                throw HandlerException("Cannot retrieve collection based on remote identifier without a resource context");
            }
            if (!qb.exec()) {
                throw HandlerException("Unable to retrieve collection for listing");
            }
            Collection::List results = qb.result();
            if (results.count() != 1) {
                throw HandlerException(QByteArray::number(results.count()) + " collections found");
            }
            col = results.first();
        } else if (mScope.scope() == Scope::HierarchicalRid) {
            if (!connection()->context()->resource().isValid()) {
                throw HandlerException("Cannot retrieve collection based on hierarchical remote identifier without a resource context");
            }
            col = CollectionQueryHelper::resolveHierarchicalRID(mScope.ridChain(), connection()->context()->resource().id());
        } else {
            throw HandlerException("WTF");
        }

        if (!col.isValid()) {
            return failureResponse("Collection " + QByteArray::number(baseCollection) + " does not exist");
        }

        if (depth == 0) { //Base listing
            collections << col;
        } else { //First level or recursive listing
            collections << retrieveChildren(col, depth);
        }
    } else { //Root folder listing
        if (depth != 0) {
            collections << retrieveChildren(Collection(), depth);
        }
    }

    Q_FOREACH (const Collection &col, collections) {
        listCollection(col, ancestorsForCollection(col));
    }

    Response response;
    response.setSuccess();
    response.setTag(tag());
    response.setString("List completed");
    Q_EMIT responseAvailable(response);
    return true;
}
