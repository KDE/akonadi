/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "notificationsource_p.h"

using namespace Akonadi;

NotificationSource::NotificationSource( QObject *source ):
  QObject( source )
{
  Q_ASSERT( source );

  connect( source, SIGNAL(notifyV2(Akonadi::NotificationMessageV2::List)),
           this, SIGNAL(notifyV2(Akonadi::NotificationMessageV2::List)) );
}

NotificationSource::~NotificationSource()
{
}

void NotificationSource::setAllMonitored( bool allMonitored )
{
  Q_ASSERT( QMetaObject::invokeMethod( parent(), "setAllMonitored",
                                       Q_ARG( bool, allMonitored ) ) );
}

void NotificationSource::setMonitoredCollection( Entity::Id id, bool monitored )
{
  Q_ASSERT( QMetaObject::invokeMethod( parent(), "setMonitoredCollection",
                                       Q_ARG( qlonglong, id ),
                                       Q_ARG( bool, monitored ) ) );
}

void NotificationSource::setMonitoredItem( Entity::Id id, bool monitored )
{
  Q_ASSERT( QMetaObject::invokeMethod( parent(), "setMonitoredItem",
                                       Q_ARG( qlonglong, id ),
                                       Q_ARG( bool, monitored ) ) );
}

void NotificationSource::setMonitoredResource( const QByteArray &resource, bool monitored )
{
  Q_ASSERT( QMetaObject::invokeMethod( parent(), "setMonitoredResource",
                                       Q_ARG( QByteArray, resource ),
                                       Q_ARG( bool, monitored ) ) );
}

void NotificationSource::setMonitoredMimeType( const QString &mimeType, bool monitored )
{
  Q_ASSERT( QMetaObject::invokeMethod( parent(), "setMonitoredMimeType",
                                       Q_ARG( QString, mimeType ),
                                       Q_ARG( bool, monitored ) ) );
}

void NotificationSource::setIgnoredSession( const QByteArray &session, bool ignored )
{
  Q_ASSERT( QMetaObject::invokeMethod( parent(), "setIgnoredSession",
                                       Q_ARG( QByteArray, session ),
                                       Q_ARG( bool, ignored ) ) );
}

QObject* NotificationSource::source() const
{
  return parent();
}
