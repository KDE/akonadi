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
#include <QtGui/QInputDialog>

#include "knutresource.h"

using namespace PIM;

KnutResource::KnutResource( const QString &id )
  : ResourceBase( id )
{
  mStatusTimer = new QTimer( this );
  connect( mStatusTimer, SIGNAL( timeout() ), this, SLOT( statusTimeout() ) );

  mSyncTimer = new QTimer( this );
  connect( mSyncTimer, SIGNAL( timeout() ), this, SLOT( syncTimeout() ) );

  int number = id.mid( 22 ).toInt();
  mStatusTimer->start( 5000 + ( 100*number ) );
}

KnutResource::~KnutResource()
{
}

void KnutResource::configure()
{
  const QString tmp = QInputDialog::getText( 0, "Configuration", "Configuration:", QLineEdit::Normal, mConfig );
  if ( !tmp.isEmpty() )
    mConfig = tmp;
}

bool KnutResource::setConfiguration( const QString &config )
{
  mConfig = config;

  return true;
}

QString KnutResource::configuration() const
{
  return mConfig;
}

void KnutResource::synchronize()
{
  mSyncTimer->start( 2000 );
}

bool KnutResource::requestItemDelivery( const QString&, const QString&, const QString&, int )
{
  return false;
}

void KnutResource::statusTimeout()
{
  static Status status = Ready;

  if ( status == Ready ) {
    changeStatus( Syncing, tr( "Syncing with Kolab Server" ) );
    status = Syncing;
  } else if ( status == Syncing ) {
    changeStatus( Error, tr( "Unable to connect to Kolab Server" ) );
    status = Error;
  } else if ( status == Error ) {
    changeStatus( Ready, tr( "Data loaded successfully from Kolab Sever" ) );
    status = Ready;
  }
}

void KnutResource::syncTimeout()
{
  mStatusTimer->stop();

  static int progress = 0;

  if ( progress == 0 )
    changeStatus( Syncing, "Syncing collection 'contacts'" );
  else if ( progress == 10 )
    changeStatus( Syncing, "Syncing collection 'events'" );
  else if ( progress == 20 )
    changeStatus( Syncing, "Syncing collection 'notes'" );
  else if ( progress == 30 )
    changeStatus( Syncing, "Syncing collection 'tasks'" );
  else if ( progress == 40 )
    changeStatus( Syncing, "Syncing collection 'journals'" );
  else if ( progress == 50 )
    changeStatus( Syncing, "Syncing collection 'mails'" );

  changeProgress( (progress % 10) * 10 );

  progress++;

  if ( progress == 50 ) {
    progress = 0;
    mSyncTimer->stop();
    changeStatus( Ready );
    changeProgress( 0 );
  }
}

#include "knutresource.moc"
