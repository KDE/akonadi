/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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


#ifndef AKONADI_IDLECLIENT_H
#define AKONADI_IDLECLIENT_H

#include <QObject>
#include <QSet>

#include "fetchhelper.h"
#include "idlemanager.h"
#include <libs/idle_p.h>

namespace Akonadi
{

class FetchScope;
class ImapSet;
class AkonadiConnection;

class IdleClient : public QObject
{
    Q_OBJECT

  public:
    explicit IdleClient( AkonadiConnection *connection, const QByteArray &clientId );
    virtual ~IdleClient();

    bool acceptsNotification( const NotificationMessageV2 &msg );
    void dispatchNotification( const Akonadi::Response &response );
    void dispatchNotification( const NotificationMessageV2 &msg,
                               const Collection &collection );

    AkonadiConnection *connection() const;
    QByteArray clientId() const;

    void freeze();
    void thaw();
    bool isFrozen() const;

    void done();

    bool replayed( const ImapSet &set );
    bool record( const ImapSet &set );

    void setRecordChanges( bool recordChanges );
    bool recordChanges();

    void setFetchScope( const FetchScope &fetchScope );
    const FetchScope& fetchScope() const;

    void setAllMonitored( bool allMonitored );
    bool isAllMonitored() const;

    void setMonitoredItems( const QSet<qint64> &items );
    void addMonitoredItems( const QSet<qint64> &items );
    void removeMonitoredItems( const QSet<qint64> &items );
    const QSet<qint64> &monitoredItems() const;

    void setMonitoredCollections( const QSet<qint64> &collections );
    void addMonitoredCollections( const QSet<qint64> &collections );
    void removeMonitoredCollections( const QSet<qint64> &collections );
    const QSet<qint64> &monitoredCollections() const;

    void setMonitoredMimeTypes( const QSet<QByteArray> &mimeTypes );
    void addMonitoredMimeTypes( const QSet<QByteArray> &mimeTypes );
    void removeMonitoredMimeTypes( const QSet<QByteArray> &mimeTypes );
    const QSet<QByteArray> &monitoredMimeTypes() const;

    void setMonitoredResources( const QSet<QByteArray> &resources );
    void addMonitoredResources( const QSet<QByteArray> &resources );
    void removeMonitoredResources( const QSet<QByteArray> &resources );
    const QSet<QByteArray> &monitoredResources() const;

    void setIgnoredSessions( const QSet<QByteArray> &sessions );
    void addIgnoredSessions( const QSet<QByteArray> &sessions );
    void removeIgnoredSessions( const QSet<QByteArray> &sessions );
    const QSet<QByteArray> &ignoredSessions() const;

    void setMonitoredOperations( const QSet<Idle::Operation> &operations );
    void addMonitoredOperations( const QSet<Idle::Operation> &operations );
    void removeMonitoredOperations( const  QSet<Idle::Operation> &operations );
    const QSet<Idle::Operation> &monitoredOperations() const;

  Q_SIGNALS:
    void responseAvailable( const Akonadi::Response &response );

  private Q_SLOTS:
    void connectionClosed();

  private:
    Q_DISABLE_COPY( IdleClient )

    // Workaround for const getters
    void updateMonitorAll() const;

    bool isCollectionMonitored( Entity::Id id ) const;
    bool isMimeTypeMonitored( const QString &mimeType ) const;
    bool isMoveDestinationResourceMonitored( const NotificationMessageV2 &msg ) const;

    AkonadiConnection *mConnection;
    QByteArray mClientId;

    bool mRecordChanges;
    FetchScope mFetchScope;

    bool mFrozen;

    mutable bool mMonitorAll;
    QSet<qint64> mMonitoredItems;
    QSet<qint64> mMonitoredCollections;
    QSet<QByteArray> mMonitoredMimeTypes;
    QSet<QByteArray> mMonitoredResources;
    QSet<Idle::Operation> mMonitoredOperations;
    QSet<QByteArray> mIgnoredSessions;
};

}

#endif // AKONADI_IDLECLIENT_H
