/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "notificationcollector.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/collectionstatistics.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "intervalcheck.h"
#include "search/searchmanager.h"
#include "akonadi.h"
#include "handler/search.h"

#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationCollector::NotificationCollector(QObject *parent)
    : QObject(parent)
    , mDb(0)
{
}

NotificationCollector::NotificationCollector(DataStore *db)
    : QObject(db)
    , mDb(db)
{
    connect(db, &DataStore::transactionCommitted, this, &NotificationCollector::transactionCommitted);
    connect(db, &DataStore::transactionRolledBack, this, &NotificationCollector::transactionRolledBack);
}

NotificationCollector::~NotificationCollector()
{
}

void NotificationCollector::itemAdded(const PimItem &item,
                                      const Collection &collection,
                                      const QByteArray &resource)
{
    SearchManager::instance()->scheduleSearchUpdate();
    itemNotification(Protocol::ItemChangeNotification::Add, item, collection, Collection(), resource);
}

void NotificationCollector::itemChanged(const PimItem &item,
                                        const QSet<QByteArray> &changedParts,
                                        const Collection &collection,
                                        const QByteArray &resource)
{
    SearchManager::instance()->scheduleSearchUpdate();
    itemNotification(Protocol::ItemChangeNotification::Modify, item, collection, Collection(), resource, changedParts);
}

void NotificationCollector::itemsFlagsChanged(const PimItem::List &items,
                                              const QSet< QByteArray > &addedFlags,
                                              const QSet< QByteArray > &removedFlags,
                                              const Collection &collection,
                                              const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyFlags, items, collection, Collection(), resource, QSet<QByteArray>(), addedFlags, removedFlags);
}

void NotificationCollector::itemsTagsChanged(const PimItem::List &items,
                                             const QSet<qint64> &addedTags,
                                             const QSet<qint64> &removedTags,
                                             const Collection &collection,
                                             const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyTags, items, collection, Collection(), resource, QSet<QByteArray>(), QSet<QByteArray>(), QSet<QByteArray>(), addedTags, removedTags);
}

void NotificationCollector::itemsRelationsChanged(const PimItem::List &items,
                                                  const Relation::List &addedRelations,
                                                  const Relation::List &removedRelations,
                                                  const Collection &collection,
                                                  const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::ModifyRelations, items, collection, Collection(), resource, QSet<QByteArray>(), QSet<QByteArray>(), QSet<QByteArray>(), QSet<qint64>(), QSet<qint64>(), addedRelations, removedRelations);
}

void NotificationCollector::itemsMoved(const PimItem::List &items,
                                       const Collection &collectionSrc,
                                       const Collection &collectionDest,
                                       const QByteArray &sourceResource)
{
    SearchManager::instance()->scheduleSearchUpdate();
    itemNotification(Protocol::ItemChangeNotification::Move, items, collectionSrc, collectionDest, sourceResource);
}

void NotificationCollector::itemsRemoved(const PimItem::List &items,
                                         const Collection &collection,
                                         const QByteArray &resource)
{
    itemNotification(Protocol::ItemChangeNotification::Remove, items, collection, Collection(), resource);
}

void NotificationCollector::itemsLinked(const PimItem::List &items, const Collection &collection)
{
    itemNotification(Protocol::ItemChangeNotification::Link, items, collection, Collection(), QByteArray());
}

void NotificationCollector::itemsUnlinked(const PimItem::List &items, const Collection &collection)
{
    itemNotification(Protocol::ItemChangeNotification::Unlink, items, collection, Collection(), QByteArray());
}

void NotificationCollector::collectionAdded(const Collection &collection,
                                            const QByteArray &resource)
{
    if (AkonadiServer::instance()->cacheCleaner()) {
        AkonadiServer::instance()->cacheCleaner()->collectionAdded(collection.id());
    }
    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->collectionAdded(collection.id());
    }
    collectionNotification(Protocol::CollectionChangeNotification::Add, collection, collection.parentId(), -1, resource);
}

void NotificationCollector::collectionChanged(const Collection &collection,
                                              const QList<QByteArray> &changes,
                                              const QByteArray &resource)
{
    if (AkonadiServer::instance()->cacheCleaner()) {
        AkonadiServer::instance()->cacheCleaner()->collectionChanged(collection.id());
    }
    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->collectionChanged(collection.id());
    }
    CollectionStatistics::self()->invalidateCollection(collection);
    collectionNotification(Protocol::CollectionChangeNotification::Modify, collection, collection.parentId(), -1, resource, changes.toSet());
}

void NotificationCollector::collectionMoved(const Collection &collection,
                                            const Collection &source,
                                            const QByteArray &resource,
                                            const QByteArray &destResource)
{
    if (AkonadiServer::instance()->cacheCleaner()) {
        AkonadiServer::instance()->cacheCleaner()->collectionChanged(collection.id());
    }
    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->collectionChanged(collection.id());
    }
    collectionNotification(Protocol::CollectionChangeNotification::Move, collection, source.id(), collection.parentId(), resource, QSet<QByteArray>(), destResource);
}

void NotificationCollector::collectionRemoved(const Collection &collection,
                                              const QByteArray &resource)
{
    if (AkonadiServer::instance()->cacheCleaner()) {
        AkonadiServer::instance()->cacheCleaner()->collectionRemoved(collection.id());
    }
    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->collectionRemoved(collection.id());
    }
    CollectionStatistics::self()->invalidateCollection(collection);
    collectionNotification(Protocol::CollectionChangeNotification::Remove, collection, collection.parentId(), -1, resource);
}

void NotificationCollector::collectionSubscribed(const Collection &collection,
                                                 const QByteArray &resource)
{
    if (AkonadiServer::instance()->cacheCleaner()) {
        AkonadiServer::instance()->cacheCleaner()->collectionAdded(collection.id());
    }
    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->collectionAdded(collection.id());
    }
    collectionNotification(Protocol::CollectionChangeNotification::Subscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>());
}

void NotificationCollector::collectionUnsubscribed(const Collection &collection,
                                                   const QByteArray &resource)
{
    if (AkonadiServer::instance()->cacheCleaner()) {
        AkonadiServer::instance()->cacheCleaner()->collectionRemoved(collection.id());
    }
    if (AkonadiServer::instance()->intervalChecker()) {
        AkonadiServer::instance()->intervalChecker()->collectionRemoved(collection.id());
    }
    CollectionStatistics::self()->invalidateCollection(collection);
    collectionNotification(Protocol::CollectionChangeNotification::Unsubscribe, collection, collection.parentId(), -1, resource, QSet<QByteArray>());
}

void NotificationCollector::tagAdded(const Tag &tag)
{
    tagNotification(Protocol::TagChangeNotification::Add, tag);
}

void NotificationCollector::tagChanged(const Tag &tag)
{
    tagNotification(Protocol::TagChangeNotification::Modify, tag);
}

void NotificationCollector::tagRemoved(const Tag &tag, const QByteArray &resource, const QString &remoteId)
{
    tagNotification(Protocol::TagChangeNotification::Remove, tag, resource, remoteId);
}

void NotificationCollector::relationAdded(const Relation &relation)
{
    relationNotification(Protocol::RelationChangeNotification::Add, relation);
}

void NotificationCollector::relationRemoved(const Relation &relation)
{
    relationNotification(Protocol::RelationChangeNotification::Remove, relation);
}

void NotificationCollector::transactionCommitted()
{
    dispatchNotifications();
}

void NotificationCollector::transactionRolledBack()
{
    clear();
}

void NotificationCollector::clear()
{
    mNotifications.clear();
}

void NotificationCollector::setSessionId(const QByteArray &sessionId)
{
    mSessionId = sessionId;
}

void NotificationCollector::itemNotification(Protocol::ItemChangeNotification::Operation op,
                                             const PimItem &item,
                                             const Collection &collection,
                                             const Collection &collectionDest,
                                             const QByteArray &resource,
                                             const QSet<QByteArray> &parts)
{
    PimItem::List items;
    items << item;
    itemNotification(op, items, collection, collectionDest, resource, parts);
}

void NotificationCollector::itemNotification(Protocol::ItemChangeNotification::Operation op,
                                             const PimItem::List &items,
                                             const Collection &collection,
                                             const Collection &collectionDest,
                                             const QByteArray &resource,
                                             const QSet<QByteArray> &parts,
                                             const QSet<QByteArray> &addedFlags,
                                             const QSet<QByteArray> &removedFlags,
                                             const QSet<qint64> &addedTags,
                                             const QSet<qint64> &removedTags,
                                             const Relation::List &addedRelations,
                                             const Relation::List &removedRelations)
{
    Collection notificationDestCollection;
    QMap<Entity::Id, QList<PimItem> > vCollections;

    if ((op == Protocol::ItemChangeNotification::Modify) ||
        (op == Protocol::ItemChangeNotification::ModifyFlags) ||
        (op == Protocol::ItemChangeNotification::ModifyTags) ||
        (op == Protocol::ItemChangeNotification::ModifyRelations)) {
        vCollections = DataStore::self()->virtualCollections(items);
    }

    Protocol::ItemChangeNotification msg;
    msg.setSessionId(mSessionId);
    msg.setOperation(op);

    msg.setItemParts(parts);
    msg.setAddedFlags(addedFlags);
    msg.setRemovedFlags(removedFlags);
    msg.setAddedTags(addedTags);
    msg.setRemovedTags(removedTags);
    if (!addedRelations.isEmpty()) {
        QSet<Protocol::ItemChangeNotification::Relation> rels;
        Q_FOREACH (const Relation &rel, addedRelations) {
            rels.insert(Protocol::ItemChangeNotification::Relation(rel.leftId(), rel.rightId(), rel.relationType().name()));
        }
        msg.setAddedRelations(rels);
    }
    if (!removedRelations.isEmpty()) {
        QSet<Protocol::ItemChangeNotification::Relation> rels;
        Q_FOREACH (const Relation &rel, removedRelations) {
            rels.insert(Protocol::ItemChangeNotification::Relation(rel.leftId(), rel.rightId(), rel.relationType().name()));
        }
        msg.setRemovedRelations(rels);
    }

    if (collectionDest.isValid()) {
        QByteArray destResourceName;
        destResourceName = collectionDest.resource().name().toLatin1();
        msg.setDestinationResource(destResourceName);
    }

    msg.setParentDestCollection(collectionDest.id());

    /* Notify all virtual collections the items are linked to. */
    auto iter = vCollections.constBegin(), endIter = vCollections.constEnd();
    for (; iter != endIter; ++iter) {
        Protocol::ItemChangeNotification copy = msg;

        Q_FOREACH (const PimItem &item, iter.value()) {
            copy.addItem(item.id(), item.remoteId(), item.remoteRevision(), item.mimeType().name());
        }
        copy.setParentCollection(iter.key());
        copy.setResource(resource);

        CollectionStatistics::self()->invalidateCollection(Collection::retrieveById(iter.key()));
        dispatchNotification(copy);
    }

    Q_FOREACH (const PimItem &item, items) {
        msg.addItem(item.id(), item.remoteId(), item.remoteRevision(), item.mimeType().name());
    }

    Collection col;
    if (!collection.isValid()) {
        msg.setParentCollection(items.first().collection().id());
        col = items.first().collection();
    } else {
        msg.setParentCollection(collection.id());
        col = collection;
    }

    QByteArray res = resource;
    if (res.isEmpty()) {
        res = col.resource().name().toLatin1();
    }
    msg.setResource(res);

    CollectionStatistics::self()->invalidateCollection(col);
    dispatchNotification(msg);
}

void NotificationCollector::collectionNotification(Protocol::CollectionChangeNotification::Operation op,
                                                   const Collection &collection,
                                                   Collection::Id source,
                                                   Collection::Id destination,
                                                   const QByteArray &resource,
                                                   const QSet<QByteArray> &changes,
                                                   const QByteArray &destResource)
{
    Protocol::CollectionChangeNotification msg;
    msg.setOperation(op);
    msg.setSessionId(mSessionId);
    msg.setId(collection.id());
    msg.setRemoteId(collection.remoteId());
    msg.setRemoteRevision(collection.remoteRevision());
    msg.setParentCollection(source);
    msg.setParentDestCollection(destination);
    msg.setDestinationResource(destResource);
    msg.setChangedParts(changes);

    if (!collection.enabled()) {
        msg.addMetadata("DISABLED");
    }

    QByteArray res = resource;
    if (res.isEmpty()) {
        res = collection.resource().name().toLatin1();
    }
    msg.setResource(res);

    dispatchNotification(msg);
}

void NotificationCollector::tagNotification(Protocol::TagChangeNotification::Operation op,
                                             const Tag &tag,
                                             const QByteArray &resource,
                                             const QString &remoteId
                                           )
{
    Protocol::TagChangeNotification msg;
    msg.setOperation(op);
    msg.setSessionId(mSessionId);
    msg.setResource(resource);
    msg.setId(tag.id());
    msg.setRemoteId(remoteId);

    dispatchNotification(msg);
}

void NotificationCollector::relationNotification(Protocol::RelationChangeNotification::Operation op,
                                                 const Relation &relation)
{
    Protocol::RelationChangeNotification msg;
    msg.setOperation(op);
    msg.setSessionId(mSessionId);
    msg.setLeftItem(relation.leftId());
    msg.setRightItem(relation.rightId());
    msg.setRemoteId(relation.remoteId());
    msg.setType(relation.relationType().name());

    dispatchNotification(msg);
}

void NotificationCollector::dispatchNotification(const Protocol::ChangeNotification &msg)
{
    if (!mDb || mDb->inTransaction()) {
        if (msg.type() == Protocol::Command::CollectionChangeNotification) {
            Protocol::CollectionChangeNotification::appendAndCompress(mNotifications, msg);
        } else {
            mNotifications.append(msg);
        }
    } else {
        Q_EMIT notify({ msg });
    }
}

void NotificationCollector::dispatchNotifications()
{
    if (!mNotifications.isEmpty()) {
        Q_EMIT notify(mNotifications);
        clear();
    }
}
