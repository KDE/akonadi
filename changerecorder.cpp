/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "changerecorder.h"
#include "monitor_p.h"

#include <kdebug.h>
#include <QSettings>

using namespace Akonadi;

class Akonadi::ChangeRecorderPrivate : public MonitorPrivate
{
  public:
    ChangeRecorderPrivate( ChangeRecorder* parent ) :
      MonitorPrivate( parent ),
      settings( 0 )
    {
    }

    Q_DECLARE_PUBLIC( ChangeRecorder )
    NotificationMessage::List pendingNotifications;
    QSettings *settings;

    virtual void slotNotify( const NotificationMessage::List &msgs )
    {
      Q_Q( ChangeRecorder );
      int oldChanges = pendingNotifications.count();
      foreach ( const NotificationMessage msg, msgs ) {
        if ( acceptNotification( msg ) )
          NotificationMessage::appendAndCompress( pendingNotifications, msg );
      }
      if ( pendingNotifications.count() != oldChanges ) {
        saveNotifications();
        emit q->changesAdded();
      }
    }

    void loadNotifications()
    {
      pendingNotifications.clear();
      settings->beginGroup( QLatin1String( "ChangeRecorder" ) );
      int size = settings->beginReadArray( QLatin1String( "change" ) );
      for ( int i = 0; i < size; ++i ) {
        settings->setArrayIndex( i );
        NotificationMessage msg;
        msg.setSessionId( settings->value( QLatin1String( "sessionId" ) ).toByteArray() );
        msg.setType( (NotificationMessage::Type)settings->value( QLatin1String( "type" ) ).toInt() );
        msg.setOperation( (NotificationMessage::Operation)settings->value( QLatin1String( "op" ) ).toInt() );
        msg.setUid( settings->value( QLatin1String( "uid" ) ).toInt() );
        msg.setRemoteId( settings->value( QLatin1String( "rid" ) ).toString() );
        msg.setResource( settings->value( QLatin1String( "resource" ) ).toByteArray() );
        msg.setParentCollection( settings->value( QLatin1String( "parentCol" ) ).toInt() );
        msg.setParentDestCollection( settings->value( QLatin1String( "parentDestCol" ) ).toInt() );
        msg.setMimeType( settings->value( QLatin1String( "mimeType" ) ).toString() );
        msg.setItemParts( settings->value( QLatin1String( "itemParts" ) ).toStringList() );
        pendingNotifications << msg;
      }
      settings->endArray();
      settings->endGroup();
    }

    void saveNotifications()
    {
      settings->beginGroup( QLatin1String( "ChangeRecorder" ) );
      settings->beginWriteArray( QLatin1String( "change" ), pendingNotifications.count() );
      for ( int i = 0; i < pendingNotifications.count(); ++i ) {
        settings->setArrayIndex( i );
        NotificationMessage msg = pendingNotifications.at( i );
        settings->setValue( QLatin1String( "sessionId" ), msg.sessionId() );
        settings->setValue( QLatin1String( "type" ), msg.type() );
        settings->setValue( QLatin1String( "op" ), msg.operation() );
        settings->setValue( QLatin1String( "uid" ), msg.uid() );
        settings->setValue( QLatin1String( "rid" ), msg.remoteId() );
        settings->setValue( QLatin1String( "resource" ), msg.resource() );
        settings->setValue( QLatin1String( "parentCol" ), msg.parentCollection() );
        settings->setValue( QLatin1String( "parentDestCol" ), msg.parentDestCollection() );
        settings->setValue( QLatin1String( "mimeType" ), msg.mimeType() );
        settings->setValue( QLatin1String( "itemParts" ), msg.itemParts() );
      }
      settings->endArray();
      settings->endGroup();
    }

};

ChangeRecorder::ChangeRecorder(QObject * parent) :
    Monitor( new ChangeRecorderPrivate( this ), parent )
{
  Q_D( ChangeRecorder );
  d->connectToNotificationManager();
}

ChangeRecorder::~ ChangeRecorder()
{
  Q_D( ChangeRecorder );
  d->saveNotifications();
}

void ChangeRecorder::setConfig(QSettings * settings)
{
  Q_D( ChangeRecorder );
  d->settings = settings;
  Q_ASSERT( d->pendingNotifications.isEmpty() );
  d->loadNotifications();
}

void ChangeRecorder::replayNext()
{
  Q_D( ChangeRecorder );
  while( !d->pendingNotifications.isEmpty() ) {
    const NotificationMessage msg = d->pendingNotifications.first();
    if ( d->processNotification( msg ) )
      break;
    d->pendingNotifications.takeFirst();
  }
  d->saveNotifications();
}

bool ChangeRecorder::isEmpty() const
{
  const Q_D( ChangeRecorder );
  return d->pendingNotifications.isEmpty();
}

void ChangeRecorder::changeProcessed()
{
  Q_D( ChangeRecorder );
  if ( !d->pendingNotifications.isEmpty() )
    d->pendingNotifications.removeFirst();
  d->saveNotifications();
}

#include "changerecorder.moc"
