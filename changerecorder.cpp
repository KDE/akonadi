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
#include <QtCore/QSettings>
#include <QtCore/QTime>

using namespace Akonadi;

class Akonadi::ChangeRecorderPrivate : public MonitorPrivate
{
  public:
    ChangeRecorderPrivate( ChangeRecorder* parent ) :
      MonitorPrivate( parent ),
      settings( 0 ),
      enableChangeRecording( true ),
      safetyTimer( 0 )
    {
    }

    Q_DECLARE_PUBLIC( ChangeRecorder )
    QSettings *settings;
    bool enableChangeRecording;
    QTimer *safetyTimer;
    QTime safetyTimeElapsed;
    QString safetyTimerMessage;

    virtual int pipelineSize() const
    {
      if ( enableChangeRecording )
        return 0; // we fill the pipeline ourselves when using change recording
      return MonitorPrivate::pipelineSize();
    }

    virtual void slotNotify( const NotificationMessage::List &msgs )
    {
      Q_Q( ChangeRecorder );
      const int oldChanges = pendingNotifications.size();
      MonitorPrivate::slotNotify( msgs ); // with change recording disabled this will automatically take care of dispatching notification messages
      if ( enableChangeRecording && pendingNotifications.size() != oldChanges ) {
        saveNotifications();
        emit q->changesAdded();
      }
    }

    void loadNotifications()
    {
      pendingNotifications.clear();
      QStringList list;
      settings->beginGroup( QLatin1String( "ChangeRecorder" ) );
      int size = settings->beginReadArray( QLatin1String( "change" ) );
      for ( int i = 0; i < size; ++i ) {
        settings->setArrayIndex( i );
        NotificationMessage msg;
        msg.setSessionId( settings->value( QLatin1String( "sessionId" ) ).toByteArray() );
        msg.setType( (NotificationMessage::Type)settings->value( QLatin1String( "type" ) ).toInt() );
        msg.setOperation( (NotificationMessage::Operation)settings->value( QLatin1String( "op" ) ).toInt() );
        msg.setUid( settings->value( QLatin1String( "uid" ) ).toLongLong() );
        msg.setRemoteId( settings->value( QLatin1String( "rid" ) ).toString() );
        msg.setResource( settings->value( QLatin1String( "resource" ) ).toByteArray() );
        msg.setParentCollection( settings->value( QLatin1String( "parentCol" ) ).toLongLong() );
        msg.setParentDestCollection( settings->value( QLatin1String( "parentDestCol" ) ).toLongLong() );
        msg.setMimeType( settings->value( QLatin1String( "mimeType" ) ).toString() );
        list = settings->value( QLatin1String( "itemParts" ) ).toStringList();
        QSet<QByteArray> itemParts;
        Q_FOREACH( const QString &entry, list )
          itemParts.insert( entry.toLatin1() );
        msg.setItemParts( itemParts );
        pendingNotifications << msg;
      }
      settings->endArray();
      settings->endGroup();
    }

    void saveNotifications()
    {
      if ( !settings )
        return;
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

        QStringList list;
        const QSet<QByteArray> itemParts = msg.itemParts();
        QSetIterator<QByteArray> it( itemParts );
        while ( it.hasNext() )
          list.append( QString::fromLatin1( it.next() ) );

        settings->setValue( QLatin1String( "itemParts" ), list );
      }
      settings->endArray();
      settings->endGroup();
    }

    void startSafetyTimer( const QString &message )
    {
      if( safetyTimer ) {
        safetyTimer->start();
        safetyTimeElapsed.restart();
        safetyTimerMessage = message;
      }
    }

    void stopSafetyTimer()
    {
      if( safetyTimer ) {
        safetyTimer->stop();
        safetyTimerMessage.clear();
      }
    }

    void safetyTimeout() // slot
    {
      kWarning() << "Change notification taking" << ( safetyTimeElapsed.elapsed() / 1000 )
        << "seconds:" << safetyTimerMessage;
    }
};

ChangeRecorder::ChangeRecorder(QObject * parent) :
    Monitor( new ChangeRecorderPrivate( this ), parent )
{
  Q_D( ChangeRecorder );
  d->init();
  d->connectToNotificationManager();

  // Warn the developer if a change notification takes more than 5 seconds to process.
  d->safetyTimer = new QTimer( this );
  d->safetyTimer->setInterval( 5000 );
  connect( d->safetyTimer, SIGNAL(timeout()), this, SLOT(safetyTimeout()) );
}

ChangeRecorder::~ ChangeRecorder()
{
  Q_D( ChangeRecorder );
  d->saveNotifications();
}

void ChangeRecorder::setConfig(QSettings * settings)
{
  Q_D( ChangeRecorder );
  if ( settings ) {
    d->settings = settings;
    Q_ASSERT( d->pendingNotifications.isEmpty() );
    d->loadNotifications();
  } else if ( d->settings ) {
    d->saveNotifications();
    d->settings = settings;
  }
}

void ChangeRecorder::replayNext()
{
  Q_D( ChangeRecorder );
  if ( !d->pendingNotifications.isEmpty() ) {
    const NotificationMessage msg = d->pendingNotifications.first();
    if ( d->ensureDataAvailable( msg ) ) {
      d->startSafetyTimer( msg.toString() );
      d->emitNotification( msg );
    } else {
      d->pipeline.enqueue( msg );
    }
  } else {
    // This is necessary when none of the notifications were accepted / processed
    // above, and so there is no one to call changeProcessed() and the ChangeReplay task
    // will be stuck forever in the ResourceScheduler.
    d->startSafetyTimer( QLatin1String( "nothingToReplay" ) );
    emit nothingToReplay();
  }
  d->saveNotifications();
}

bool ChangeRecorder::isEmpty() const
{
  Q_D( const ChangeRecorder );
  return d->pendingNotifications.isEmpty();
}

void ChangeRecorder::changeProcessed()
{
  Q_D( ChangeRecorder );
  d->stopSafetyTimer();
  if ( !d->pendingNotifications.isEmpty() )
    d->pendingNotifications.removeFirst();
  d->saveNotifications();
}

void ChangeRecorder::setChangeRecordingEnabled( bool enable )
{
  Q_D( ChangeRecorder );
  d->enableChangeRecording = enable;
  Q_ASSERT( enable || d->pendingNotifications.isEmpty() );
}

#include "changerecorder.moc"
