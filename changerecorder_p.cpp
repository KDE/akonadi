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

#include "changerecorder_p.h"
using namespace Akonadi;

ChangeRecorderPrivate::ChangeRecorderPrivate(ChangeNotificationDependenciesFactory *dependenciesFactory_,
                                             ChangeRecorder *parent)
  : MonitorPrivate( dependenciesFactory_, parent ),
     settings( 0 ),
     enableChangeRecording( true ),
    m_lastKnownNotificationsCount( 0 ),
    m_startOffset( 0 ),
    m_needFullSave( true )
{
}

int ChangeRecorderPrivate::pipelineSize() const
{
  if ( enableChangeRecording )
    return 0; // we fill the pipeline ourselves when using change recording
  return MonitorPrivate::pipelineSize();
}

void ChangeRecorderPrivate::slotNotify(const Akonadi::NotificationMessage::List &msgs)
{
  Q_Q( ChangeRecorder );
  const int oldChanges = pendingNotifications.size();
  // with change recording disabled this will automatically take care of dispatching notification messages and saving
  MonitorPrivate::slotNotify( msgs );
  if ( enableChangeRecording && pendingNotifications.size() != oldChanges ) {
    emit q->changesAdded();
  }
}

bool ChangeRecorderPrivate::emitNotification(const Akonadi::NotificationMessage &msg)
{
  const bool someoneWasListening = MonitorPrivate::emitNotification( msg );
  if ( !someoneWasListening && enableChangeRecording )
    QMetaObject::invokeMethod( q_ptr, "replayNext", Qt::QueuedConnection ); // skip notifications no one was listening to
  return someoneWasListening;
}

// The QSettings object isn't actually used anymore, except for migrating old data
// and it gives us the base of the filename to use. This is all historical.
QString ChangeRecorderPrivate::notificationsFileName() const
{
  return settings->fileName() + QLatin1String( "_changes.dat" );
}

void ChangeRecorderPrivate::loadNotifications()
{
    pendingNotifications.clear();
    Q_ASSERT(pipeline.isEmpty());
    pipeline.clear();

    const QString changesFileName = notificationsFileName();

    /**
     * In an older version we recorded changes inside the settings object, however
     * for performance reasons we changed that to store them in a separated file.
     * If this file doesn't exists, it means we run the new version the first time,
     * so we have to read in the legacy list of changes first.
     */
    if ( !QFile::exists( changesFileName ) ) {
      QStringList list;
      settings->beginGroup( QLatin1String( "ChangeRecorder" ) );
      const int size = settings->beginReadArray( QLatin1String( "change" ) );

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

      // save notifications to the new file...
      saveNotifications();

      // ...delete the legacy list...
      settings->remove( QString() );
      settings->endGroup();

      // ...and continue as usually
    }

    QFile file( changesFileName );
    if ( file.open( QIODevice::ReadOnly ) ) {
      m_needFullSave = false;
      pendingNotifications = loadFrom( &file );
    } else {
      m_needFullSave = true;
    }
    notificationsLoaded();
}

static const quint64 s_currentVersion = Q_UINT64_C(0x000100000000);
static const quint64 s_versionMask    = Q_UINT64_C(0xFFFF00000000);
static const quint64 s_sizeMask       = Q_UINT64_C(0x0000FFFFFFFF);

QQueue<NotificationMessage> ChangeRecorderPrivate::loadFrom(QIODevice *device)
{
  QDataStream stream( device );
  stream.setVersion( QDataStream::Qt_4_6 );

  QByteArray sessionId, resource;
  int type, operation;
  quint64 uid, parentCollection, parentDestCollection;
  QString remoteId, mimeType;
  QSet<QByteArray> itemParts;

  QQueue<NotificationMessage> list;

  quint64 sizeAndVersion;
  stream >> sizeAndVersion;

  const quint64 size = sizeAndVersion & s_sizeMask;
  const quint64 version = (sizeAndVersion & s_versionMask) >> 32;

  quint64 startOffset = 0;
  if ( version >= 1 ) {
    stream >> startOffset;
  }

  // If we skip the first N items, then we'll need to rewrite the file on saving.
  // Also, if the file is old, it needs to be rewritten.
  m_needFullSave = startOffset > 0 || version == 0;

  for ( quint64 i = 0; i < size && !stream.atEnd(); ++i ) {
    NotificationMessage msg;

    stream >> sessionId;
    stream >> type;
    stream >> operation;
    stream >> uid;
    stream >> remoteId;
    stream >> resource;
    stream >> parentCollection;
    stream >> parentDestCollection;
    stream >> mimeType;
    stream >> itemParts;

    if ( i < startOffset )
      continue;

    msg.setSessionId( sessionId );
    msg.setType( static_cast<NotificationMessage::Type>( type ) );
    msg.setOperation( static_cast<NotificationMessage::Operation>( operation ) );
    msg.setUid( uid );
    msg.setRemoteId( remoteId );
    msg.setResource( resource );
    msg.setParentCollection( parentCollection );
    msg.setParentDestCollection( parentDestCollection );
    msg.setMimeType( mimeType );
    msg.setItemParts( itemParts );
    list << msg;
  }
  return list;
}

QString ChangeRecorderPrivate::dumpNotificationListToString() const
{
  if ( !settings )
    return QString::fromLatin1( "No settings set in ChangeRecorder yet." );
  QString result;
  const QString changesFileName = notificationsFileName();
  QFile file( changesFileName );
  if ( !file.open( QIODevice::ReadOnly ) )
    return QString::fromLatin1( "Error reading " ) + changesFileName;

  QDataStream stream( &file );
  stream.setVersion( QDataStream::Qt_4_6 );

  QByteArray sessionId, resource;
  int type, operation;
  quint64 uid, parentCollection, parentDestCollection;
  QString remoteId, mimeType;
  QSet<QByteArray> itemParts;

  QStringList list;

  quint64 sizeAndVersion;
  stream >> sizeAndVersion;

  const quint64 size = sizeAndVersion & s_sizeMask;
  const quint64 version = (sizeAndVersion & s_versionMask) >> 32;

  quint64 startOffset = 0;
  if ( version >= 1 ) {
    stream >> startOffset;
  }

  for ( quint64 i = 0; i < size && !stream.atEnd(); ++i ) {
    stream >> sessionId;
    stream >> type;
    stream >> operation;
    stream >> uid;
    stream >> remoteId;
    stream >> resource;
    stream >> parentCollection;
    stream >> parentDestCollection;
    stream >> mimeType;
    stream >> itemParts;

    if ( i < startOffset )
        continue;

    QString typeString;
    switch ( type ) {
    case NotificationMessage::Collection:
      typeString = QLatin1String( "Collection" );
      break;
    case NotificationMessage::Item:
      typeString = QLatin1String( "Item" );
      break;
    default:
      typeString = QLatin1String( "InvalidType" );
      break;
    };

    QString operationString;
    switch ( operation ) {
    case NotificationMessage::Add:
      operationString = QLatin1String( "Add" );
      break;
    case NotificationMessage::Modify:
      operationString = QLatin1String( "Modify" );
      break;
    case NotificationMessage::Move:
      operationString = QLatin1String( "Move" );
      break;
    case NotificationMessage::Remove:
      operationString = QLatin1String( "Remove" );
      break;
    case NotificationMessage::Link:
      operationString = QLatin1String( "Link" );
      break;
    case NotificationMessage::Unlink:
      operationString = QLatin1String( "Unlink" );
      break;
    case NotificationMessage::Subscribe:
      operationString = QLatin1String( "Subscribe" );
      break;
    case NotificationMessage::Unsubscribe:
      operationString = QLatin1String( "Unsubscribe" );
      break;
    default:
      operationString = QLatin1String( "InvalidOp" );
      break;
    };

    QStringList itemPartsList;
    foreach( const QByteArray &b, itemParts )
      itemPartsList.push_back( QString::fromLatin1(b) );

    const QString entry = QString::fromLatin1("session=%1 type=%2 operation=%3 uid=%4 remoteId=%5 resource=%6 parentCollection=%7 parentDestCollection=%8 mimeType=%9 itemParts=%10")
                          .arg( QString::fromLatin1( sessionId ) )
                          .arg( typeString )
                          .arg( operationString )
                          .arg( uid )
                          .arg( remoteId )
                          .arg( QString::fromLatin1( resource ) )
                          .arg( parentCollection )
                          .arg( parentDestCollection )
                          .arg( mimeType )
                          .arg( itemPartsList.join(QLatin1String(", " )) );

    result += entry + QLatin1Char('\n');
  }

  return result;
}

void ChangeRecorderPrivate::addToStream(QDataStream &stream, const NotificationMessage &msg)
{
  stream << msg.sessionId();
  stream << int(msg.type());
  stream << int(msg.operation());
  stream << quint64(msg.uid());
  stream << msg.remoteId();
  stream << msg.resource();
  stream << quint64(msg.parentCollection());
  stream << quint64(msg.parentDestCollection());
  stream << msg.mimeType();
  stream << msg.itemParts();
}

void ChangeRecorderPrivate::writeStartOffset()
{
  if ( !settings )
    return;

  QFile file( notificationsFileName() );
  if ( !file.open( QIODevice::ReadWrite ) ) {
    qWarning() << "Could not update notifications in file" << file.fileName();
    return;
  }

  // Skip "countAndVersion"
  file.seek( 8 );

  //kDebug() << "Writing start offset=" << m_startOffset;

  QDataStream stream( &file );
  stream.setVersion( QDataStream::Qt_4_6 );
  stream << static_cast<quint64>(m_startOffset);

  // Everything else stays unchanged
}

void ChangeRecorderPrivate::saveNotifications()
{
  if ( !settings )
    return;

  QFile file( notificationsFileName() );
  QFileInfo info( file );
  if ( !QFile::exists( info.absolutePath() ) ) {
    QDir dir;
    dir.mkpath( info.absolutePath() );
  }
  if ( !file.open( QIODevice::WriteOnly ) ) {
    qWarning() << "Could not save notifications to file" << file.fileName();
    return;
  }
  saveTo(&file);
  m_needFullSave = false;
  m_startOffset = 0;
}

void ChangeRecorderPrivate::saveTo(QIODevice *device)
{
  // Version 0 of this file format was writing a quint64 count, followed by the notifications.
  // Version 1 bundles a version number into that quint64, to be able to detect a version number at load time.

  const quint64 countAndVersion = static_cast<quint64>(pendingNotifications.count()) | s_currentVersion;

  QDataStream stream( device );
  stream.setVersion( QDataStream::Qt_4_6 );

  stream << countAndVersion;
  stream << quint64(0); // no start offset

  //kDebug() << "Saving" << pendingNotifications.count() << "notifications (full save)";

  for ( int i = 0; i < pendingNotifications.count(); ++i ) {
    const NotificationMessage msg = pendingNotifications.at( i );
    addToStream( stream, msg );
  }
}

void ChangeRecorderPrivate::notificationsEnqueued( int count )
{
  // Just to ensure the contract is kept, and these two methods are always properly called.
  if (enableChangeRecording) {
    m_lastKnownNotificationsCount += count;
    if ( m_lastKnownNotificationsCount != pendingNotifications.count() ) {
      kWarning() << this << "The number of pending notifications changed without telling us! Expected"
                 << m_lastKnownNotificationsCount << "but got" << pendingNotifications.count()
                 << "Caller just added" << count;
      Q_ASSERT( pendingNotifications.count() == m_lastKnownNotificationsCount );
    }

    saveNotifications();
  }
}

void ChangeRecorderPrivate::dequeueNotification()
{
  pendingNotifications.dequeue();

  if (enableChangeRecording) {

    Q_ASSERT( pendingNotifications.count() == m_lastKnownNotificationsCount - 1 );
    --m_lastKnownNotificationsCount;

    if ( m_needFullSave || pendingNotifications.isEmpty() ) {
      saveNotifications();
    } else {
      ++m_startOffset;
      writeStartOffset();
    }
  }
}

void ChangeRecorderPrivate::notificationsErased()
{
  if (enableChangeRecording) {
    m_lastKnownNotificationsCount = pendingNotifications.count();
    m_needFullSave = true;
    saveNotifications();
  }
}

void ChangeRecorderPrivate::notificationsLoaded()
{
  m_lastKnownNotificationsCount = pendingNotifications.count();
  m_startOffset = 0;
}
