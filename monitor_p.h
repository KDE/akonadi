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

#include "monitor.h"
#include "collection.h"
#include "collectionstatusjob.h"
#include "item.h"
#include "job.h"
#include "notificationmanagerinterface.h"

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
    QSet<int> items;
    QSet<QString> mimetypes;
    bool monitorAll;
    QList<QByteArray> sessions;
    QStringList mFetchParts;
    QHash<KJob*,NotificationMessage> pendingJobs;

    bool isCollectionMonitored( int collection, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( uint item, int collection, int collectionDest,
                          const QString &mimetype, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) ||
           isCollectionMonitored( collectionDest ) ||items.contains( item ) ||
           resources.contains( resource ) || mimetypes.contains( mimetype ) )
        return true;
      return false;
    }

    bool isSessionIgnored( const QByteArray &sessionId ) const
    {
      return sessions.contains( sessionId );
    }

    bool connectToNotificationManager();
    bool processNotification( const NotificationMessage &msg );

    // private slots
    void sessionDestroyed( QObject* );
    void slotStatusChangedFinished( KJob* );
    void slotFlushRecentlyChangedCollections();

    virtual void slotNotify( const NotificationMessage::List &msgs );
    void slotItemJobFinished( KJob* job );
    void slotCollectionJobFinished( KJob *job );

    void emitItemNotification( const NotificationMessage &msg, const Item &item = Item(),
                               const Collection &collection = Collection(), const Collection &collectionDest = Collection() );
    void emitCollectionNotification( const NotificationMessage &msg, const Collection &col = Collection(),
                                     const Collection &par = Collection() );

    bool fetchCollection;
    bool fetchCollectionStatus;
    bool fetchAllParts;

  private:
    // collections that need a status update
    QSet<int> recentlyChangedCollections;

    bool isCollectionMonitored( int collection ) const
    {
      if ( collections.contains( Collection( collection ) ) )
        return true;
      if ( collections.contains( Collection::root() ) )
        return true;
      return false;
    }

    void fetchStatus( int colId )
    {
      CollectionStatusJob *job = new CollectionStatusJob( Collection( colId ), q_ptr );
      QObject::connect( job, SIGNAL(result(KJob*)), q_ptr, SLOT(slotStatusChangedFinished(KJob*)) );
    }

    void notifyCollectionStatusWatchers( int collection, const QByteArray &resource )
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
    explicit ItemCollectionFetchJob( const DataReference &reference, int collectionId, QObject *parent = 0 );
    ~ItemCollectionFetchJob();

    Item item() const;
    Collection collection() const;

    void addFetchPart( const QString &identifier );
    void fetchAllParts();

  protected:
    virtual void doStart();

  private Q_SLOTS:
    void collectionJobDone( KJob* job );
    void itemJobDone( KJob* job );

  private:
    DataReference mReference;
    int mCollectionId;

    Item mItem;
    Collection mCollection;
    QStringList mFetchParts;
    bool mFetchAllParts;
};

}

#endif
