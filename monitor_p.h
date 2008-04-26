/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_MONITOR_P_H
#define AKONADI_MONITOR_P_H

// @cond PRIVATE

#include "monitor.h"
#include "collection.h"
#include "collectionstatisticsjob.h"
#include "item.h"
#include "itemfetchscope.h"
#include "job.h"
#include "notificationmanagerinterface.h"

#include <kmimetype.h>

#include <QObject>
#include <QTimer>

namespace Akonadi {

class Monitor;

class MonitorPrivate
{
  public:
    MonitorPrivate( Monitor *parent );
    virtual ~MonitorPrivate() {}

    Monitor *q_ptr;
    Q_DECLARE_PUBLIC( Monitor )
    org::kde::Akonadi::NotificationManager *nm;
    Collection::List collections;
    QSet<QByteArray> resources;
    QSet<Item::Id> items;
    QSet<QString> mimetypes;
    bool monitorAll;
    QList<QByteArray> sessions;
    ItemFetchScope mItemFetchScope;
    QHash<KJob*,NotificationMessage> pendingJobs;

    bool isCollectionMonitored( Collection::Id collection, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( Item::Id item, Collection::Id collection, Collection::Id collectionDest,
                          const QString &mimetype, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) ||
           isCollectionMonitored( collectionDest ) ||items.contains( item ) ||
           resources.contains( resource ) || isMimeTypeMonitored( mimetype ) )
        return true;
      return false;
    }

    bool isSessionIgnored( const QByteArray &sessionId ) const
    {
      return sessions.contains( sessionId );
    }

    bool connectToNotificationManager();
    bool acceptNotification( const NotificationMessage &msg );
    bool processNotification( const NotificationMessage &msg );

    // private slots
    void sessionDestroyed( QObject* );
    void slotStatisticsChangedFinished( KJob* );
    void slotFlushRecentlyChangedCollections();

    virtual void slotNotify( const NotificationMessage::List &msgs );
    void slotItemJobFinished( KJob* job );
    void slotCollectionJobFinished( KJob *job );

    void emitItemNotification( const NotificationMessage &msg, const Item &item = Item(),
                               const Collection &collection = Collection(), const Collection &collectionDest = Collection() );
    void emitCollectionNotification( const NotificationMessage &msg, const Collection &col = Collection(),
                                     const Collection &par = Collection() );

    bool fetchCollection;
    bool fetchCollectionStatistics;

  private:
    // collections that need a statistics update
    QSet<Collection::Id> recentlyChangedCollections;

    bool isCollectionMonitored( Collection::Id collection ) const
    {
      if ( collections.contains( Collection( collection ) ) )
        return true;
      if ( collections.contains( Collection::root() ) )
        return true;
      return false;
    }

    bool isMimeTypeMonitored( const QString& mimetype ) const
    {
      if ( mimetypes.contains( mimetype ) )
        return true;

      KMimeType::Ptr mimeType = KMimeType::mimeType( mimetype, KMimeType::ResolveAliases );
      if ( mimeType.isNull() )
        return false;

      foreach ( const QString &mt, mimetypes ) {
        if ( mimeType->is( mt ) )
          return true;
      }

      return false;
    }

    void fetchStatistics( Collection::Id colId )
    {
      CollectionStatisticsJob *job = new CollectionStatisticsJob( Collection( colId ), q_ptr );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotStatisticsChangedFinished(KJob*)) );
    }

    void notifyCollectionStatisticsWatchers( Collection::Id collection, const QByteArray &resource )
    {
      if ( isCollectionMonitored( collection, resource ) ) {
        if (recentlyChangedCollections.empty() )
          QTimer::singleShot( 500, q_ptr, SLOT(slotFlushRecentlyChangedCollections()) );
        recentlyChangedCollections.insert( collection );
      }
    }
};


/**
  A job which fetches a job and a collection.
 */
class AKONADI_EXPORT ItemCollectionFetchJob : public Job
{
  Q_OBJECT

  public:
    explicit ItemCollectionFetchJob( const Item &item, Collection::Id collectionId, QObject *parent = 0 );
    ~ItemCollectionFetchJob();

    Item item() const;
    Collection collection() const;

    void setFetchScope( const ItemFetchScope &fetchScope );

  protected:
    virtual void doStart();

  private Q_SLOTS:
    void collectionJobDone( KJob* job );
    void itemJobDone( KJob* job );

  private:
    Item mReferenceItem;
    Collection::Id mCollectionId;

    Item mItem;
    Collection mCollection;
    ItemFetchScope mFetchScope;
};

}

// @endcond

#endif
