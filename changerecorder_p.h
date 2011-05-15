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

#ifndef AKONADI_CHANGERECORDER_P_H
#define AKONADI_CHANGERECORDER_P_H

#include "akonadiprivate_export.h"
#include "monitor_p.h"

class AKONADI_TESTS_EXPORT Akonadi::ChangeRecorderPrivate : public Akonadi::MonitorPrivate
{
  public:
    ChangeRecorderPrivate( MonitorDependeciesFactory *dependenciesFactory_, ChangeRecorder* parent ) :
      MonitorPrivate( dependenciesFactory_, parent ),
      settings( 0 ),
      enableChangeRecording( true )
    {
    }

    Q_DECLARE_PUBLIC( ChangeRecorder )
    QSettings *settings;
    bool enableChangeRecording;

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

    virtual bool emitNotification(const Akonadi::NotificationMessage& msg)
    {
      const bool someoneWasListening = MonitorPrivate::emitNotification( msg );
      if ( !someoneWasListening && enableChangeRecording )
        QMetaObject::invokeMethod( q_ptr, "replayNext", Qt::QueuedConnection ); // skip notifications noone was listening to
      return someoneWasListening;
    }

    void loadNotifications()
    {
      pendingNotifications.clear();

      const QString changesFileName = settings->fileName() + QLatin1String( "_changes.dat" );

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
      if ( !file.open( QIODevice::ReadOnly ) )
        return;
      pendingNotifications = loadFrom( &file );
    }

    QQueue<NotificationMessage> loadFrom( QIODevice *device )
    {
      QDataStream stream( device );
      stream.setVersion( QDataStream::Qt_4_6 );

      qulonglong size;
      QByteArray sessionId, resource;
      int type, operation;
      qlonglong uid, parentCollection, parentDestCollection;
      QString remoteId, mimeType;
      QSet<QByteArray> itemParts;

      QQueue<NotificationMessage> list;

      stream >> size;
      for ( qulonglong i = 0; i < size; ++i ) {
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

    void addToStream( QDataStream &stream, const NotificationMessage &msg )
    {
        stream << msg.sessionId();
        stream << msg.type();
        stream << msg.operation();
        stream << msg.uid();
        stream << msg.remoteId();
        stream << msg.resource();
        stream << msg.parentCollection();
        stream << msg.parentDestCollection();
        stream << msg.mimeType();
        stream << msg.itemParts();
    }

    void saveNotifications()
    {
      if ( !settings )
        return;

      QFile file( settings->fileName() + QLatin1String( "_changes.dat" ) );
      QFileInfo info( file );
      if ( !QFile::exists( info.absolutePath() ) ) {
        QDir dir;
        dir.mkpath( info.absolutePath() );
      }
      if ( !file.open( QIODevice::WriteOnly ) ) {
        qWarning() << "could not save notifications to file " << file.fileName();
        return;
      }
      saveTo(&file);
    }

    void saveTo( QIODevice *device )
    {

      QDataStream stream( device );
      stream.setVersion( QDataStream::Qt_4_6 );

      stream << (qulonglong)(pipeline.count() + pendingNotifications.count());

      for ( int i = 0; i < pipeline.count(); ++i ) {
        const NotificationMessage msg = pipeline.at( i );
        addToStream( stream, msg );
      }

      for ( int i = 0; i < pendingNotifications.count(); ++i ) {
        const NotificationMessage msg = pendingNotifications.at( i );
        addToStream( stream, msg );
      }
    }
};

#endif
