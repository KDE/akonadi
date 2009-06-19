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

#include "entities.h"

#include "../../libs/notificationmessage_p.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

namespace Akonadi {

class DataStore;

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
    void itemAdded( const PimItem &item, const Collection &collection = Collection(),
                    const QString &mimeType = QString(),
                    const QByteArray &resource = QByteArray() );

    /**
      Notify about a changed item.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemChanged( const PimItem &item,
                      const QSet<QByteArray> &changedParts,
                      const Collection &collection = Collection(),
                      const QString &mimeType = QString(),
                      const QByteArray &resource = QByteArray() );

    /**
      Notify about a moved item
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void itemMoved( const PimItem &item, const Collection &collectionSrc = Collection(),
                    const Collection &collectionDest = Collection(),
                    const QString &mimeType = QString(),
                    const QByteArray &resource = QByteArray() );

    /**
      Notify about a removed item.
      Make sure you either provide all parameters or call this function before
      actually removing the item from database.
    */
    void itemRemoved( const PimItem &item, const Collection &collection = Collection(),
                      const QString &mimeType = QString(),
                      const QByteArray &resource = QByteArray() );

    /**
     * Notify about a linked item.
     */
    void itemLinked( const PimItem &item, const Collection &collection );

    /**
     * Notify about a unlinked item.
     */
    void itemUnlinked( const PimItem &item, const Collection &collection );

    /**
      Notify about a added collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
     */
    void collectionAdded( const Collection &collection,
                          const QByteArray &resource = QByteArray() );

    /**
      Notify about a changed collection.
      Provide as many parameters as you have at hand currently, everything
      that is missing will be looked up in the database later.
    */
    void collectionChanged( const Collection &collection,
                            const QList<QByteArray> &changes,
                            const QByteArray &resource = QByteArray() );

    /**
      Notify about a removed collection.
      Make sure you either provide all parameters or call this function before
      actually removing the item from database.
     */
    void collectionRemoved( const Collection &collection,
                            const QByteArray &resource = QByteArray() );

  Q_SIGNALS:
    void notify( const Akonadi::NotificationMessage::List &msgs );

  private:
    void itemNotification( NotificationMessage::Operation op, const PimItem &item,
                           const Collection &collection,
                           const Collection &collectionDest,
                           const QString &mimeType,
                           const QByteArray &resource,
                           const QSet<QByteArray> &parts = QSet<QByteArray>() );
    void collectionNotification( NotificationMessage::Operation op,
                                 const Collection &collection,
                                 const QByteArray &resource,
                                 const QSet<QByteArray> &changes = QSet<QByteArray>() );
    void dispatchNotification( const NotificationMessage &msg );
    void clear();

  private Q_SLOTS:
    void transactionCommitted();
    void transactionRolledBack();

  private:
    DataStore *mDb;
    QByteArray mSessionId;

    NotificationMessage::List mNotifications;
};


}

#endif
