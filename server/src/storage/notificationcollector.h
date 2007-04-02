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

#ifndef AKONADI_NOTIFICATIONCOLLECTOR_H
#define AKONADI_NOTIFICATIONCOLLECTOR_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <entities.h>

namespace Akonadi {

class DataStore;

/**
  This class is used to store notifications within the NotificationCollector.
*/
class NotificationItem
{
  public:
    enum Type {
      Item,
      Collection
    };

    typedef QList<NotificationItem> List;

    /**
      Creates a notification item for PIM items.
    */
    NotificationItem( const PimItem &item, const Location &collection,
                      const QString &mimeType, const QByteArray &resource );

    /**
      Creates a notification item for collections.
    */
    NotificationItem( const Location &collection, const QByteArray &resource );

    /**
      Returns the type of this item.
    */
    Type type() const { return mType; }

    /**
      Returns the uid of the changed item.
    */
    int uid() const { return mType == Item ? mItem.id() : mCollection.id(); }

    /**
      Returns the remote id of the changed item.
    */
    QString remoteId() const { return mType == Item ? QString::fromLatin1( mItem.remoteId() ) : mCollection.remoteId(); }

    /**
      Returns the PimItem of the changed item.
    */
    PimItem pimItem() const { return mItem; }

    /**
      Returns the changed collection.
    */
    Location collection() const { return mCollection; }

    /**
      Sets the changed collection.
    */
    void setCollection( const Location &collection ) { mCollection = collection; }

    /**
      Returns the mime-type of the changed item.
    */
    QString mimeType() const { return mMimeType; }

    /**
      Sets the mimetype of the changed item.
    */
    void setMimeType( const QString &mimeType ) { mMimeType = mimeType; }

    /**
      Returns the resource of the changed collection/item.
    */
    QByteArray resource() const { return mResource; }

    /**
      Sets the resource of the changed collection/item.
    */
    void setResource( const QByteArray &resource ) { mResource = resource; }

    /**
      Returns true if all necessary information are available.
    */
    bool isComplete() const;

    /**
      Compares two NotificationItem objects.
    */
    bool operator==( const NotificationItem &item ) const;

  private:
    Type mType;
    PimItem mItem;
    Location mCollection;
    QString mMimeType;
    QByteArray mResource;
    QString mCollectionName;
};


/**
  Part of the DataStore, collects change notifications and emits
  them after the current transaction has been successfully committed.
  Where possible, notifications are compressed.
*/
class NotificationCollector : public QObject
{
  Q_OBJECT

  public:
    /**
      Create a new notification collector for the given DataStore @p db.
      @param db The datastore using this notification collector.
    */
    NotificationCollector( DataStore *db );

    /**
      Destroys this notification collector.
    */
    ~NotificationCollector();

    /**
      Sets the identifier of the session causing the changes.
    */
    void setSessionId( const QByteArray &sessionId );

    /**
      Notify about an added item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemAdded( const PimItem &item, const Location &collection = Location(),
                    const QString &mimeType = QString(),
                    const QByteArray &resource = QByteArray() );

    /**
      Notify about a changed item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemChanged( const PimItem &item, const Location &collection = Location(),
                      const QString &mimeType = QString(),
                      const QByteArray &resource = QByteArray() );

    /**
      Notify about a removed item.
      Make sure you either provide all parameters or call this function before
      actually removing the item from database.
    */
    void itemRemoved( const PimItem &item, const Location &collection = Location(),
                      const QString &mimeType = QString(),
                      const QByteArray &resource = QByteArray() );

    /**
      Notify about a added collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
     */
    void collectionAdded( const Location &collection,
                          const QByteArray &resource = QByteArray() );

    /**
      Notify about a changed collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void collectionChanged( const Location &collection,
                            const QByteArray &resource = QByteArray() );

    /**
      Notify about a removed collection.
      Make sure you either provide all parameters or call this function before
      actually removing the item from database.
     */
    void collectionRemoved( const Location &collection,
                            const QByteArray &resource = QByteArray() );

  Q_SIGNALS:
    void itemAddedNotification( const QByteArray &sessionId, int uid,
                                const QString &remotedId,
                                int collection,
                                const QString &mimeType,
                                const QByteArray &resource );
    void itemChangedNotification( const QByteArray &sessionId, int uid,
                                  const QString &remotedId,
                                  int collection,
                                  const QString &mimeType,
                                  const QByteArray &resource );
    void itemRemovedNotification( const QByteArray &sessionId, int uid,
                                  const QString &remotedId,
                                  int collection,
                                  const QString &mimeType,
                                  const QByteArray &resource );

    void collectionAddedNotification( const QByteArray &sessionId, int collection,
                                      const QString &remoteId, const QByteArray &resource );
    void collectionChangedNotification( const QByteArray &sessionId, int collection,
                                        const QString &remoteId, const QByteArray &resource );
    void collectionRemovedNotification( const QByteArray &sessionId, int collection,
                                        const QString &remoteId, const QByteArray &resource );

  private:
    void completeItem( NotificationItem &item );
    void clear();

  private Q_SLOTS:
    void transactionCommitted();
    void transactionRolledBack();

  private:
    DataStore *mDb;
    QByteArray mSessionId;

    NotificationItem::List mAddedItems;
    NotificationItem::List mChangedItems;
    NotificationItem::List mRemovedItems;

    NotificationItem::List mAddedCollections;
    NotificationItem::List mChangedCollections;
    NotificationItem::List mRemovedCollections;
};


}

#endif
