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
        if (mAncestors.contains(parent.parentId())) {
            parent = mAncestors.value(parent.parentId());
        } else {
            parent = mCollections.value(parent.parentId());
        }
        if (!parent.isValid()) {
            qWarning() << col.id();
            throw HandlerException("Found invalid parent in ancestors");
        }
        ancestors.prepend(parent);
    }
    return ancestors;
}

CollectionAttribute::List List::getAttributes(const Collection &col, const QVector<QByteArray> &filter)
{
    CollectionAttribute::List attributes;
    if (filter.contains("REMOTEID")) {
        CollectionAttribute attr;
        attr.setType("REMOTEID");
        attr.setValue(col.remoteId().toUtf8());
        attributes << attr;
    }
    if (filter.contains("NAME")) {
        CollectionAttribute attr;
        attr.setType("NAME");
        attr.setValue(col.name().toUtf8());
        attributes << attr;
    }
    {
        auto it = mCollectionAttributes.find(col.id());
        while (it != mCollectionAttributes.end() && it.key() == col.id()) {
            if (filter.isEmpty() || filter.contains(it.value().type())) {
                attributes << it.value();
            }
            ++it;
        }
    }
    return attributes;
}

void List::listCollection(const Collection &root, const QStack<Collection> &ancestors, const QList<QByteArray> &mimeTypes, const CollectionAttribute::List &attributes)
{
    const bool isReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(root.id(), connection()->sessionId());
    //We always expose referenced collections to the resource as referenced (although it's a different session)
    //Otherwise syncing wouldn't work.
    const bool resourceIsSynchronizing = root.referenced() && mCollectionsToSynchronize && connection()->context()->resource().isValid();

    QStack<CollectionAttribute::List> ancestorAttributes;
    //backwards compatibilty, collectionToByteArray will automatically fall-back to id + remoteid
    if (!mAncestorAttributes.isEmpty()) {
        Q_FOREACH (const Collection &col, ancestors) {
            ancestorAttributes.push(getAttributes(col, mAncestorAttributes));
        }
    }

    // write out collection details
    Collection dummy = root;
    DataStore *db = connection()->storageBackend();
    db->activeCachePolicy(dummy);
    const QByteArray b = HandlerHelper::collectionToByteArray(dummy, attributes, mIncludeStatistics, mAncestorDepth, ancestors, ancestorAttributes, isReferencedFromSession || resourceIsSynchronizing, mimeTypes);

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

static QSqlQuery getAttributeQuery(const QVariantList &ids, const QVector<QByteArray> &requestedAttributes)
{
    QueryBuilder qb(CollectionAttribute::tableName());

    qb.addValueCondition(CollectionAttribute::collectionIdFullColumnName(), Query::In, ids);

    qb.addColumn(CollectionAttribute::collectionIdFullColumnName());
    qb.addColumn(CollectionAttribute::typeFullColumnName());
    qb.addColumn(CollectionAttribute::valueFullColumnName());
    
    if (!requestedAttributes.isEmpty()) {
        QVariantList attributes;
        Q_FOREACH (const QByteArray &type, requestedAttributes) {
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
            attr.setType(attributeQuery.value(1).toString().toUtf8());
            attr.setValue(attributeQuery.value(2).toString().toUtf8());
            // qDebug() << "found attribute " << attr.type() << attr.value();
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

        if (!mMimeTypes.isEmpty()) {
            qb.addJoin(QueryBuilder::LeftJoin, CollectionMimeTypeRelation::tableName(), CollectionMimeTypeRelation::leftColumn(), Collection::idFullColumnName());
            QVariantList mimeTypeFilter;
            Q_FOREACH(MimeType::Id mtId, mMimeTypes) {
                mimeTypeFilter << mtId;
            }
            qb.addValueCondition(CollectionMimeTypeRelation::rightColumn(), Query::In, mimeTypeFilter);
            qb.addGroupColumn(Collection::idFullColumnName());
        }

        // qb.addSortColumn(Collection::idFullColumnName(), Query::Ascending);

        if (!qb.exec()) {
            throw HandlerException("Unable to retrieve collection for listing");
        }
        Q_FOREACH (const Collection &col, qb.result()) {
            mCollections.insert(col.id(), col);
        }
    }

    //Post filtering that we couldn't do as part of the sql query
    if (topParent.isValid() && depth > 0) {
        auto it = mCollections.begin();
        while (it != mCollections.end()) {
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

            ++it;
        }
    }

    const bool listFilterEnabled = mEnabledCollections || mCollectionsToIndex || mCollectionsToDisplay || mCollectionsToSynchronize;

    //If we matched referenced collecions we need to ensure the collection was referenced from this session
    if (listFilterEnabled) {
        auto it = mCollections.begin();
        while (it != mCollections.end()) {
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
            it++;
        }
    }

    QVariantList mimeTypeIds;
    mimeTypeIds.reserve(mCollections.keys().size());
    Q_FOREACH (const Collection::Id id, mCollections.keys()) {
        mimeTypeIds << id;
    }

    QVariantList ancestorIds;
    Q_FOREACH (const Collection::Id id, mCollections.keys()) {
        ancestorIds << id;
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
        Q_FOREACH (const Collection &col, mCollections) {
            if (col.parentId() != parentId && !mCollections.contains(col.parentId())) {
                missingCollections.insert(col.parentId());
            }
        }
    }

    // qDebug() << "HAS:" << knownIds;
    // qDebug() << "MISSING:" << missingCollections;

    //Fetch missing collections that are part of the tree
    while (!missingCollections.isEmpty()) {
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

        missingCollections.clear();
        Q_FOREACH (const Collection &missingCol, qb.result()) {
            mCollections.insert(missingCol.id(), missingCol);
            ancestorIds << missingCol.id();
            //We have to do another round if the parents parent is missing
            if (missingCol.parentId() != parentId && !mCollections.contains(missingCol.parentId())) {
                missingCollections.insert(missingCol.parentId());
            }
        }
    }

    if (!mAncestorAttributes.isEmpty()) {
        retrieveAttributes(ancestorIds);
    }

    //We are querying in batches because something can't handle WHERE IN queries with sets larger than 999
    const int querySizeLimit = 999;
    int mimetypeQueryStart = 0;
    int attributeQueryStart = 0;
    QSqlQuery mimeTypeQuery;
    QSqlQuery attributeQuery;
    auto it = mCollections.begin();
    while (it != mCollections.end()) {
        const Collection col = it.value();
        // qDebug() << "col " << col.id();

        QList<QByteArray> mimeTypes;
        {
            //Get new query if necessary
            if (!mimeTypeQuery.isValid() && mimetypeQueryStart < mimeTypeIds.size()) {
                const QVariantList ids = mimeTypeIds.mid(mimetypeQueryStart, querySizeLimit);
                mimetypeQueryStart += querySizeLimit;
                mimeTypeQuery = getMimeTypeQuery(ids);
                mimeTypeQuery.next(); //place at first record
            }

            // qDebug() << mimeTypeQuery.isValid() << mimeTypeQuery.value(0).toLongLong();
            while (mimeTypeQuery.isValid() && mimeTypeQuery.value(0).toLongLong() < col.id()) {
                qDebug() << "skipped: " << mimeTypeQuery.value(0).toLongLong() << mimeTypeQuery.value(2).toString();
                if (!mimeTypeQuery.next()) {
                    break;
                }
            }
            //Advance query while a mimetype for this collection is returned
            while (mimeTypeQuery.isValid() && mimeTypeQuery.value(0).toLongLong() == col.id()) {
                mimeTypes << mimeTypeQuery.value(2).toString().toUtf8();
                if (!mimeTypeQuery.next()) {
                    break;
                }
            }
        }

        CollectionAttribute::List attributes;
        {
            //Get new query if necessary
            if (!attributeQuery.isValid() && attributeQueryStart < mimeTypeIds.size()) {
                const QVariantList ids = mimeTypeIds.mid(attributeQueryStart, querySizeLimit);
                attributeQueryStart += querySizeLimit;
                attributeQuery = getAttributeQuery(ids, QVector<QByteArray>());
                attributeQuery.next(); //place at first record
            }

            // qDebug() << attributeQuery.isValid() << attributeQuery.value(0).toLongLong();
            while (attributeQuery.isValid() && attributeQuery.value(0).toLongLong() < col.id()) {
                qDebug() << "skipped: " << attributeQuery.value(0).toLongLong() << attributeQuery.value(1).toString();
                if (!attributeQuery.next()) {
                    break;
                }
            }
            //Advance query while a mimetype for this collection is returned
            while (attributeQuery.isValid() && attributeQuery.value(0).toLongLong() == col.id()) {
                CollectionAttribute attr;
                attr.setType(attributeQuery.value(1).toString().toUtf8());
                attr.setValue(attributeQuery.value(2).toString().toUtf8());
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
                if (m_streamParser->hasList()) {
                    m_streamParser->beginList();
                    while (!m_streamParser->atListEnd()) {
                        const QByteArray &value = m_streamParser->readString();
                        if (value == "DEPTH") {
                            mAncestorDepth = HandlerHelper::parseDepth(m_streamParser->readString());
                        } else {
                            mAncestorAttributes << value;
                        }
                    }
                } else { //Backwards compatibility
                    mAncestorDepth = HandlerHelper::parseDepth(argument);
                }
            }
        }
    }

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

        retrieveCollections(col, depth);
    } else { //Root folder listing
        if (depth != 0) {
            retrieveCollections(Collection(), depth);
        }
    }

    Response response;
    response.setSuccess();
    response.setTag(tag());
    response.setString("List completed");
    Q_EMIT responseAvailable(response);
    return true;
}
