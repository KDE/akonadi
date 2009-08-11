/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "transportresourcebase.h"
#include "transportresourcebase_p.h"

#include "transportadaptor.h"

#include <QDBusConnection>

using namespace Akonadi;

TransportResourceBasePrivate::TransportResourceBasePrivate( TransportResourceBase *qq )
  : QObject(), q( qq )
{
  new TransportAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Transport" ),
                                                this, QDBusConnection::ExportAdaptors );
}

void TransportResourceBasePrivate::send( Item::Id message )
{
  q->sendItem( message );
}

TransportResourceBase::TransportResourceBase()
  : d( new TransportResourceBasePrivate( this ) )
{
}

TransportResourceBase::~TransportResourceBase()
{
  delete d;
}

void TransportResourceBase::emitTransportResult( Item::Id &item, bool success,
                                                 const QString &message )
{
  emit d->transportResult( item, success, message );
}

#include "transportresourcebase_p.moc"
