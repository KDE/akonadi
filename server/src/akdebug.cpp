/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    Inspired by kdelibs/kdecore/io/kdebug.h
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
                  2002 Holger Freyther (freyther@kde.org)

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

#include "akdebug.h"
#include "../libs/xdgbasedirs_p.h"
using Akonadi::XdgBaseDirs;

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>

#include <cassert>

class FileDebugStream : public QIODevice
{
  public:
    FileDebugStream( const QString &fileName, QtMsgType type ) :
      mFileName( fileName ),
      mType( type )
    {
      open( WriteOnly );
    }

    bool isSequential() const { return true; }
    qint64 readData(char *, qint64) { return 0;  }
    qint64 readLineData(char *, qint64) { return 0; }
    qint64 writeData(const char *data, qint64 len)
    {
      QByteArray buf = QByteArray::fromRawData(data, len);

      QFile outputFile( mFileName );
      outputFile.open( QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered );
      outputFile.write( data, len );
      outputFile.putChar( '\n' );
      outputFile.close();

      qt_message_output( mType, buf.trimmed().constData() );
      return len;
    }

    void setType( QtMsgType type )
    {
      mType = type;
    }
  private:
    QString mFileName;
    QtMsgType mType;
};

class DebugPrivate
{
  public:
    DebugPrivate() :
      fileStream( new FileDebugStream( errorLogFileName(), QtCriticalMsg ) )
    {
    }

    ~DebugPrivate()
    {
      delete fileStream;
    }

    QString errorLogFileName() const
    {
      return XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) )
          + QDir::separator()
          + QString::fromLatin1( "akonadiserver.error" );
    }

    QDebug stream( QtMsgType type )
    {
      QMutexLocker locker( &mutex );
      if ( type == QtDebugMsg )
        return qDebug();
      fileStream->setType( type );
      return QDebug( fileStream );
    }

    QMutex mutex;
    FileDebugStream *fileStream;
};

Q_GLOBAL_STATIC( DebugPrivate, sInstance )

QDebug akFatal()
{
  return sInstance()->stream( QtFatalMsg );
}

QDebug akError()
{
  return sInstance()->stream( QtCriticalMsg );
}

QDebug akDebug()
{
  return sInstance()->stream( QtDebugMsg );
}

void akInitLog()
{
  QFileInfo infoOld( sInstance()->errorLogFileName() + QString::fromLatin1(".old") );
  if ( infoOld.exists() ) {
    QFile fileOld( infoOld.absoluteFilePath() );
    assert( fileOld.remove() );
  }
  QFileInfo info( sInstance()->errorLogFileName() );
  if ( info.exists() ) {
    QFile file( info.absoluteFilePath() );
    assert( file.rename( sInstance()->errorLogFileName() + QString::fromLatin1(".old") ) );
  }
}
