/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include <QtCore/QTimer>

#include "knutresource.h"

using namespace PIM;

KnutResource::KnutResource( const QString &id )
  : ResourceBase( id )
{
  QTimer *timer = new QTimer( this );

  connect( timer, SIGNAL( timeout() ), this, SLOT( timeout() ) );

  int number = id.mid( 22 ).toInt();
  timer->start( 5000 + ( 100*number ) );
}

KnutResource::~KnutResource()
{
}

bool KnutResource::requestItemDelivery( const QString&, const QString&, int )
{
  return false;
}

void KnutResource::timeout()
{
  static Status status = Ready;

  if ( status == Ready ) {
    statusChanged( Syncing, tr( "Syncing with Kolab Server" ) );
    status = Syncing;
  } else if ( status == Syncing ) {
    statusChanged( Error, tr( "Unable to connect to Kolab Server" ) );
    status = Error;
  } else if ( status == Error ) {
    statusChanged( Ready, tr( "Data loaded successfully from Kolab Sever" ) );
    status = Ready;
  }
}

#include "knutresource.moc"
