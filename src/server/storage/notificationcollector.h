/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "entities.h"

#include <private/protocol_p.h>

#include <QByteArray>
#include <QList>
#include <QString>

namespace Akonadi
{
namespace Server
{
class DataStore;
class Connection;
class AkonadiServer;

/**
  Part of the DataStore, collects change notifications and emits
  them after the current transaction has been successfully committed.
  Where possible, notifications are compressed.
*/
class NotificationCollector
{
public:
    /**
      Create a new notification collector for the given DataStore @p db.
      @param db The datastore using this notification collector.
    */
    explicit NotificationCollector(AkonadiServer &akonadi, DataStore *db);

    /**
      Destroys this notification collector.
    */
    virtual ~NotificationCollector() = default;

    /**
     * Sets the connection that is causing the changes.
     */
    void setConnection(Connection *connection);

    /**
      Notify about an added item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemAdded(const PimItem &item, bool seen, const Collection &collection = Collection(), const QByteArray &resource = QByteArray());

    /**
      Notify about a changed item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemChanged(const PimItem &item,
                     const QSet<QByteArray> &changedParts,
                     const Collection &collection = Collection(),
                     const QByteArray &resource = QByteArray());

    /**
      Notify about changed items flags
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemsFlagsChanged(const PimItem::List &items,
                           const QSet<QByteArray> &addedFlags,
                           const QSet<QByteArray> &removedFlags,
                           const Collection &collection = Collection(),
                           const QByteArray &resource = QByteArray());

    /**
     Notify about changed items tags
    **/
    void itemsTagsChanged(const PimItem::List &items,
                          const QSet<qint64> &addedTags,
                          const QSet<qint64> &removedTags,
                          const Collection &collection = Collection(),
                          const QByteArray &resource = QByteArray());

    /**
     Notify about changed items relations
    **/
    void itemsRelationsChanged(const PimItem::List &items,
                               const Relation::List &addedRelations,
                               const Relation::List &removedRelations,
                               const Collection &collection = Collection(),
                               const QByteArray &resource = QByteArray());

    /**
      Notify about moved items
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemsMoved(const PimItem::List &items,
                    const Collection &collectionSrc = Collection(),
                    const Collection &collectionDest = Collection(),
                    const QByteArray &sourceResource = QByteArray());

    /**
      Notify about removed items.
      Make sure you either provide all parameters or call this function before
      actually removing the item from database.
    */
    void itemsRemoved(const PimItem::List &items, const Collection &collection = Collection(), const QByteArray &resource = QByteArray());

    /**
     * Notify about linked items
     */
    void itemsLinked(const PimItem::List &items, const Collection &collection);

    /**
     * Notify about unlinked items.
     */
    void itemsUnlinked(const PimItem::List &items, const Collection &collection);

    /**
      Notify about a added collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
     */
    void collectionAdded(const Collection &collection, const QByteArray &resource = QByteArray());

    /**
      Notify about a changed collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void collectionChanged(const Collection &collection, const QList<QByteArray> &changes, const QByteArray &resource = QByteArray());

    /**
      Notify about a moved collection.
      Provide as many parameters as you have at hand currently, everything
      missing will be looked up on demand in the database later.
    */
    void collectionMoved(const Collection &collection,
                         const Collection &source,
                         const QByteArray &resource = QByteArray(),
                         const QByteArray &destResource = QByteArray());

    /**
      Notify about a removed collection.
      Make sure you either provide all parameters or call this function before
      actually removing the item from database.
     */
    void collectionRemoved(const Collection &collection, const QByteArray &resource = QByteArray());

    /**
     *      Notify about a collection subscription.
     */
    void collectionSubscribed(const Collection &collection, const QByteArray &resource = QByteArray());
    /**
     *      Notify about a collection unsubscription
     */
    void collectionUnsubscribed(const Collection &collection, const QByteArray &resource = QByteArray());

    /**
      Notify about an added tag.
     */
    void tagAdded(const Tag &tag);

    /**
     Notify about a changed tag.
     */
    void tagChanged(const Tag &tag);

    /**
      Notify about a removed tag.
     */
    void tagRemoved(const Tag &tag, const QByteArray &resource, const QString &remoteId);

    /**
      Notify about an added relation.
     */
    void relationAdded(const Relation &relation);

    /**
      Notify about a removed relation.
     */
    void relationRemoved(const Relation &relation);

    /**
      Trigger sending of collected notifications.

      @returns Returns true when any notifications were dispatched, false if there
               were no pending notifications.
    */
    bool dispatchNotifications();

private:
    void itemNotification(Protocol::ItemChangeNotification::Operation op,
                          const PimItem::List &items,
                          const Collection &collection,
                          const Collection &collectionDest,
                          const QByteArray &resource,
                          const QSet<QByteArray> &parts = QSet<QByteArray>(),
                          const QSet<QByteArray> &addedFlags = QSet<QByteArray>(),
                          const QSet<QByteArray> &removedFlags = QSet<QByteArray>(),
                          const QSet<qint64> &addedTags = QSet<qint64>(),
                          const QSet<qint64> &removedTags = QSet<qint64>(),
                          const Relation::List &addedRelations = Relation::List(),
                          const Relation::List &removedRelations = Relation::List());
    void itemNotification(Protocol::ItemChangeNotification::Operation op,
                          const PimItem &item,
                          const Collection &collection,
                          const Collection &collectionDest,
                          const QByteArray &resource,
                          const QSet<QByteArray> &parts = QSet<QByteArray>());
    void collectionNotification(Protocol::CollectionChangeNotification::Operation op,
                                const Collection &collection,
                                Collection::Id source,
                                Collection::Id destination,
                                const QByteArray &resource,
                                const QSet<QByteArray> &changes = QSet<QByteArray>(),
                                const QByteArray &destResource = QByteArray());
    void tagNotification(Protocol::TagChangeNotification::Operation op,
                         const Tag &tag,
                         const QByteArray &resource = QByteArray(),
                         const QString &remoteId = QString());
    void relationNotification(Protocol::RelationChangeNotification::Operation op, const Relation &relation);
    void dispatchNotification(const Protocol::ChangeNotificationPtr &msg);
    void clear();

    void completeNotification(const Protocol::ChangeNotificationPtr &msg);

protected:
    virtual void notify(Protocol::ChangeNotificationList &&ntfs);

private:
    Q_DISABLE_COPY_MOVE(NotificationCollector)

    DataStore *mDb;
    Connection *mConnection = nullptr;
    AkonadiServer &mAkonadi;
    bool mIgnoreTransactions = false;

    Protocol::ChangeNotificationList mNotifications;
};

} // namespace Server
} // namespace Akonadi
