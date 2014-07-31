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

bool List::listCollection(const Collection &root, int depth, const QStack<Collection> &ancestors)
{
    // recursive listing of child collections
    bool childrenFound = false;
    if (depth > 0) {
        const Collection::List children = retrieveChildren(root.id());
        QStack<Collection> ancestorsAndMe(ancestors);
        ancestorsAndMe.push(root);
        Q_FOREACH (const Collection &col, children) {
            if (listCollection(col, depth - 1, ancestorsAndMe)) {
                childrenFound = true;
            }
        }
    }

    const bool isReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(root.id(), connection()->sessionId());
    //The collection is referenced, but not from this session.
    if (root.referenced() && !isReferencedFromSession) {
        //Don't include the collection when only looking for enabled collections
        if (mEnabledCollections && !root.enabled()) {
            return false;
        }
        //Don't include the collection when only looking for collections to display/index/sync
        if (mCollectionsToDisplay &&
            (((root.displayPref() == Tristate::Undefined) && !root.enabled()) ||
             (root.displayPref() == Tristate::False))) {
            return false;
        }
        if (mCollectionsToIndex &&
            (((root.indexPref() == Tristate::Undefined) && !root.enabled()) ||
             (root.indexPref() == Tristate::False))) {
            return false;
        }
        //Single collection sync will still work since that is using a base fetch
        if (mCollectionsToSynchronize &&
            (((root.syncPref() == Tristate::Undefined) && !root.enabled()) ||
             (root.syncPref() == Tristate::False))) {
            return false;
        }
    }

    // filter if this node isn't needed by it's children
    const bool hidden = (mResource.isValid() && root.resourceId() != mResource.id())
                        || (!mMimeTypes.isEmpty() && !intersect(mMimeTypes, root.mimeTypes()));

    if (!childrenFound && hidden) {
        return false;
    }

    // write out collection details
    Collection dummy = root;
    DataStore *db = connection()->storageBackend();
    db->activeCachePolicy(dummy);
    const QByteArray b = HandlerHelper::collectionToByteArray(dummy, hidden, mIncludeStatistics, mAncestorDepth, ancestors, isReferencedFromSession);

    Response response;
    response.setUntagged();
    response.setString(b);
    Q_EMIT responseAvailable(response);

    return true;
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

Collection::List List::retrieveChildren(const QVariant &value)
{
    SelectQueryBuilder<Collection> qb;
    if (value.isNull()) {
        qb.addValueCondition(Collection::parentIdColumn(), Query::Is, QVariant());
    } else {
        qb.addValueCondition(Collection::parentIdColumn(), Query::Equals, value);
    }

    if (mEnabledCollections) {
        Query::Condition orCondition(Query::Or);
        orCondition.addValueCondition(Collection::enabledFullColumnName(), Query::Equals, true);
        orCondition.addValueCondition(Collection::referencedFullColumnName(), Query::Equals, true);
        qb.addCondition(orCondition);
    } else if (mCollectionsToSynchronize) {
        qb.addCondition(filterCondition(Collection::syncPrefFullColumnName()));
    } else if (mCollectionsToDisplay) {
        qb.addCondition(filterCondition(Collection::displayPrefFullColumnName()));
    } else if (mCollectionsToIndex) {
        qb.addCondition(filterCondition(Collection::indexPrefFullColumnName()));
    }
    if (!qb.exec()) {
        throw HandlerException("Unable to retrieve collection for listing");
    }
    return qb.result();
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

    int depth = HandlerHelper::parseDepth(m_streamParser->readString());

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
            ancestors = ancestorsForCollection(col);
        } else { //First level or recursive listing
            collections << retrieveChildren(col.id());
            --depth;
            if (!collections.isEmpty()) {
                ancestors = ancestorsForCollection(collections.first());
            }
        }
    } else { //Root folder listing
        if (depth != 0) {
            Collection::List list = retrieveChildren(QVariant());
            collections << list;
        }
        --depth;
    }

    Q_FOREACH (const Collection &col, collections) {
        listCollection(col, depth, ancestors);
    }

    Response response;
    response.setSuccess();
    response.setTag(tag());
    response.setString("List completed");
    Q_EMIT responseAvailable(response);
    return true;
}
