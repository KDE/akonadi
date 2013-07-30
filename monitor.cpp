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

#include "monitor.h"
#include "monitor_p.h"

#include "changemediator_p.h"
#include "collectionfetchscope.h"
#include "itemfetchjob.h"
#include "session.h"

#include <kdebug.h>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <iterator>

using namespace Akonadi;

Monitor::Monitor( QObject *parent ) :
    QObject( parent ),
    d_ptr( new MonitorPrivate( 0, this ) )
{
  d_ptr->init();
  d_ptr->connectToNotificationManager();
}

//@cond PRIVATE
Monitor::Monitor(MonitorPrivate * d, QObject *parent) :
    QObject( parent ),
    d_ptr( d )
{
  d_ptr->init();
  d_ptr->connectToNotificationManager();

  ChangeMediator::registerMonitor(this);
}
//@endcond

Monitor::~Monitor()
{
  ChangeMediator::unregisterMonitor(this);

  // :TODO: Unsubscribe from the notification manager. That means having some kind of reference
  // counting on the server side.
  delete d_ptr;
}

void Monitor::setCollectionMonitored( const Collection &collection, bool monitored )
{
  Q_D( Monitor );
  if ( monitored ) {
    d->collections << collection;
  } else {
    d->collections.removeAll( collection );
    d->cleanOldNotifications();
  }
  emit collectionMonitored( collection, monitored );
}

void Monitor::setItemMonitored( const Item & item, bool monitored )
{
  Q_D( Monitor );
  if ( monitored ) {
    d->items.insert( item.id() );
  } else {
    d->items.remove( item.id() );
    d->cleanOldNotifications();
  }
  emit itemMonitored( item,  monitored );
}

void Monitor::setResourceMonitored( const QByteArray & resource, bool monitored )
{
  Q_D( Monitor );
  if ( monitored ) {
    d->resources.insert( resource );
  } else {
    d->resources.remove( resource );
    d->cleanOldNotifications();
  }
  emit resourceMonitored( resource, monitored );
}

void Monitor::setMimeTypeMonitored( const QString & mimetype, bool monitored )
{
  Q_D( Monitor );
  if ( monitored ) {
    d->mimetypes.insert( mimetype );
  } else {
    d->mimetypes.remove( mimetype );
    d->cleanOldNotifications();
  }

  emit mimeTypeMonitored( mimetype, monitored );
}

void Akonadi::Monitor::setAllMonitored( bool monitored )
{
  Q_D( Monitor );
  d->monitorAll = monitored;

  if ( !monitored ) {
    d->cleanOldNotifications();
  }

  emit allMonitored( monitored );
}

void Monitor::ignoreSession(Session * session)
{
  Q_D( Monitor );
  d->sessions << session->sessionId();
  connect( session, SIGNAL(destroyed(QObject*)), this, SLOT(slotSessionDestroyed(QObject*)) );
}

void Monitor::fetchCollection(bool enable)
{
  Q_D( Monitor );
  d->fetchCollection = enable;
}

void Monitor::fetchCollectionStatistics(bool enable)
{
  Q_D( Monitor );
  d->fetchCollectionStatistics = enable;
}

void Monitor::setItemFetchScope( const ItemFetchScope &fetchScope )
{
  Q_D( Monitor );
  d->mItemFetchScope = fetchScope;
}

ItemFetchScope &Monitor::itemFetchScope()
{
  Q_D( Monitor );
  return d->mItemFetchScope;
}

void Monitor::fetchChangedOnly( bool enable )
{
  Q_D( Monitor );
  d->mFetchChangedOnly = enable;
}

void Monitor::setCollectionFetchScope( const CollectionFetchScope &fetchScope )
{
  Q_D( Monitor );
  d->mCollectionFetchScope = fetchScope;
}

CollectionFetchScope& Monitor::collectionFetchScope()
{
  Q_D( Monitor );
  return d->mCollectionFetchScope;
}

Akonadi::Collection::List Monitor::collectionsMonitored() const
{
  Q_D( const Monitor );
  return d->collections;
}

QList<Item::Id> Monitor::itemsMonitored() const
{
  Q_D( const Monitor );
  return d->items.toList();
}

QVector<Item::Id> Monitor::itemsMonitoredEx() const
{
  Q_D( const Monitor );
  QVector<Item::Id> result;
  result.reserve( d->items.size() );
  qCopy( d->items.begin(), d->items.end(), std::back_inserter( result ) );
  return result;
}

QStringList Monitor::mimeTypesMonitored() const
{
  Q_D( const Monitor );
  return d->mimetypes.toList();
}

QList<QByteArray> Monitor::resourcesMonitored() const
{
  Q_D( const Monitor );
  return d->resources.toList();
}

bool Monitor::isAllMonitored() const
{
  Q_D( const Monitor );
  return d->monitorAll;
}

void Monitor::setSession( Akonadi::Session *session )
{
  Q_D( Monitor );
  if (session == d->session)
    return;

  if (!session)
    d->session = Session::defaultSession();
  else
    d->session = session;

  d->itemCache->setSession(d->session);
  d->collectionCache->setSession(d->session);
}

Session* Monitor::session() const
{
  Q_D( const Monitor );
  return d->session;
}

void Monitor::setCollectionMoveTranslationEnabled( bool enabled )
{
  Q_D( Monitor );
  d->collectionMoveTranslationEnabled = enabled;
}

#include "moc_monitor.cpp"
