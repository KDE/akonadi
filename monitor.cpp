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

#include "collectionlistjob.h"
#include "itemfetchjob.h"
#include "notificationmessage.h"
#include "session.h"

#include <kdebug.h>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

using namespace Akonadi;

#define d d_ptr

Monitor::Monitor( QObject *parent ) :
    QObject( parent ),
    d_ptr( new MonitorPrivate( this ) )
{
  d_ptr->connectToNotificationManager();
}

//@cond PRIVATE
Monitor::Monitor(MonitorPrivate * d, QObject *parent) :
    QObject( parent ),
    d_ptr( d )
{
}
//@endcond

Monitor::~Monitor()
{
  delete d;
}

void Monitor::monitorCollection( const Collection &collection )
{
  d->collections << collection;
}

void Monitor::monitorItem( const DataReference & ref )
{
  d->items.insert( ref.id() );
}

void Monitor::monitorResource(const QByteArray & resource)
{
  d->resources.insert( resource );
}

void Monitor::monitorMimeType(const QString & mimetype)
{
  d->mimetypes.insert( mimetype );
}

void Akonadi::Monitor::monitorAll()
{
  d->monitorAll = true;
}

void Monitor::ignoreSession(Session * session)
{
  d->sessions << session->sessionId();
}

void Monitor::fetchCollection(bool enable)
{
  d->fetchCollection = enable;
}

void Monitor::addFetchPart( const QString &identifier )
{
  if ( !d->mFetchParts.contains( identifier ) )
    d->mFetchParts.append( identifier );
}

void Monitor::fetchCollectionStatus(bool enable)
{
  d->fetchCollectionStatus = enable;
}

void Monitor::fetchAllParts()
{
  d->fetchAllParts = true;
}

#undef d

#include "monitor.moc"
