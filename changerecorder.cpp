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
#include "changerecorder_p.h"

#include <kdebug.h>
#include <QtCore/QSettings>

using namespace Akonadi;

ChangeRecorder::ChangeRecorder( QObject * parent ) :
    Monitor( new ChangeRecorderPrivate( 0, this ), parent )
{
}

ChangeRecorder::ChangeRecorder( ChangeRecorderPrivate *privateclass, QObject * parent ) :
    Monitor( privateclass, parent )
{
}

ChangeRecorder::~ChangeRecorder()
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
    const NotificationMessage msg = d->pendingNotifications.head();
    if ( d->ensureDataAvailable( msg ) )
      d->emitNotification( msg );
    else if ( !d->translateAndCompress( d->pipeline, msg ) ) {
      // In the case of a move where both source and destination are
      // ignored, we ignore the message and process the next one.
      d->pendingNotifications.dequeue();
      return replayNext();
    }
  } else {
    // This is necessary when none of the notifications were accepted / processed
    // above, and so there is no one to call changeProcessed() and the ChangeReplay task
    // will be stuck forever in the ResourceScheduler.
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
  if ( !d->pendingNotifications.isEmpty() )
    d->pendingNotifications.dequeue();
  d->saveNotifications();
}

void ChangeRecorder::setChangeRecordingEnabled( bool enable )
{
  Q_D( ChangeRecorder );
  if ( d->enableChangeRecording == enable )
    return;
  d->enableChangeRecording = enable;
  if ( !enable )
    d->dispatchNotifications();
}

#include "changerecorder.moc"
