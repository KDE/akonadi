/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <QByteArray>
#include <QList>
#include <QObject>

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
    NotificationItem( int uid, const QByteArray &collection,
                      const QByteArray &mimeType, const QByteArray &resource );

    /**
      Creates a notification item for collections.
    */
    NotificationItem( const QByteArray &collection, const QByteArray &resource );

    /**
      Returns the type of this item.
    */
    Type type() const { return mType; }

    /**
      Returns the uid of the changed item.
    */
    int uid() const { return mUid; }

    /**
      Returns the changed collection.
    */
    QByteArray collection() const { return mCollection; }

    /**
      Sets the changed collection.
    */
    void setCollection( const QByteArray &collection ) { mCollection = collection; }

    /**
      Returns the mime-type of the changed item.
    */
    QByteArray mimeType() const { return mMimeType; }

    /**
      Sets the mimetype of the changed item.
    */
    void setMimeType( const QByteArray &mimeType ) { mMimeType = mimeType; }

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
    int mUid;
    QByteArray mCollection;
    QByteArray mMimeType;
    QByteArray mResource;
};


/**
  Part of the DataStore, collects change notifications and emits
  them after the current transaction has been successfully committed.
  Where possible, notifications are compressed.

  @todo The itemRemoved() (and maybe other) signals also needs the remoteid,
        maybe we should use DataReference everywhere?
*/
class NotificationCollector : public QObject
{
  Q_OBJECT

  public:
    /**
      Create a new notification collector for the given DataStore @p db.
    */
    NotificationCollector( DataStore *db );

    /**
      Destroys this notification collector.
    */
    ~NotificationCollector();

    /**
      Notify about an added item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemAdded( int uid, const QByteArray &collection = QByteArray(),
                    const QByteArray &mimeType = QByteArray(),
                    const QByteArray &resource = QByteArray() );

    /**
      Notify about a changed item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemChanged( int uid, const QByteArray &collection = QByteArray(),
                      const QByteArray &mimeType = QByteArray(),
                      const QByteArray &resource = QByteArray() );

    /**
      Notify about a removed item.
      In contrast to itemAdded() and itemChanged() the caller must
      provide all required information for obvious reasons.
    */
    void itemRemoved( int uid, const QByteArray &collection,
                      const QByteArray &mimeType, const QByteArray &resource );

    /**
      Notify about a added collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void collectionAdded( const QByteArray &collection,
                          const QByteArray &resource = QByteArray() );

    /**
      Notify about a changed collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void collectionChanged( const QByteArray &collection,
                            const QByteArray &resource = QByteArray() );

    /**
      Notify about a removed collection.
      In contrast to collectionAdded() and collectionChanged() the caller must
      provide all required information for obvious reasons.
    */
    void collectionRemoved( const QByteArray &collection,
                            const QByteArray &resource );

  signals:
    void itemAddedNotification( int uid, const QByteArray &collection,
                                const QByteArray &mimeType,
                                const QByteArray &resource );
    void itemChangedNotification( int uid, const QByteArray &collection,
                                  const QByteArray &mimeType,
                                  const QByteArray &resource );
    void itemRemovedNotification( int uid, const QByteArray &collection,
                                  const QByteArray &mimeType,
                                  const QByteArray &resource );

    void collectionAddedNotification( const QByteArray &collection,
                                      const QByteArray &resource );
    void collectionChangedNotification( const QByteArray &collection,
                                        const QByteArray &resource );
    void collectionRemovedNotification( const QByteArray &collection,
                                        const QByteArray &resource );

  private:
    void completeItem( NotificationItem &item );

  private slots:
    void transactionCommitted();
    void transactionRolledBack();

  private:
    DataStore *mDb;

    NotificationItem::List mAddedItems;
    NotificationItem::List mChangedItems;
    NotificationItem::List mRemovedItems;

    NotificationItem::List mAddedCollections;
    NotificationItem::List mChangedCollections;
    NotificationItem::List mRemovedCollections;
};


}

#endif
