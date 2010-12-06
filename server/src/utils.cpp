/*
 * Copyright (C) 2010 Tobias Koenig <tokoe@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "utils.h"

#include "libs/xdgbasedirs_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtNetwork/QHostInfo>

#include <cerrno>
#include <cstdlib>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

static QString akonadiSocketDirectory();
static bool checkSocketDirectory( const QString &path );
static bool createSocketDirectory( const QString &link, const QString &tmpl );

using namespace Akonadi;

QString Utils::preferredSocketDirectory( const QString &defaultDirectory )
{
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  const QSettings serverSettings( serverConfigFile, QSettings::IniFormat );

  QString socketDir = defaultDirectory;
  if ( !serverSettings.contains( QLatin1String( "Connection/SocketDirectory" ) ) ) {
    // if no socket directory is defined, use the symlinked from /tmp
    socketDir = akonadiSocketDirectory();

    if ( socketDir.isEmpty() ) // if that does not work, fall back on default
      socketDir = defaultDirectory;
  } else {
    socketDir = serverSettings.value( QLatin1String( "Connection/SocketDirectory" ), defaultDirectory ).toString();
  }

  const QString userName = QString::fromLocal8Bit( qgetenv( "USER" ) );
  if ( socketDir.contains( QLatin1String( "$USER" ) ) && !userName.isEmpty() )
    socketDir.replace( QLatin1String( "$USER" ), userName );

  if ( socketDir[0] != QLatin1Char( '/' ) ) {
    QDir::home().mkdir( socketDir );
    socketDir = QDir::homePath() + QLatin1Char( '/' ) + socketDir;
  }

  QFileInfo dirInfo( socketDir );
  if ( !dirInfo.exists() )
    QDir::home().mkpath( dirInfo.absoluteFilePath() );

  return socketDir;
}

QString akonadiSocketDirectory()
{
  const QString hostname = QHostInfo::localHostName();

  if ( hostname.isEmpty() ) {
    qCritical() << "QHostInfo::localHostName() failed";
    return QString();
  }

  const uid_t uid = getuid();
  const struct passwd *pw_ent = getpwuid( uid );
  if ( !pw_ent ) {
    qCritical() << "Could not get passwd entry for user id" << uid;
    return QString();
  }

  const QString link = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) ) + QLatin1Char( '/' ) + QLatin1String( "socket-" ) + hostname;
  const QString tmpl = QLatin1String( "akonadi-" ) + QLatin1String( pw_ent->pw_name ) + QLatin1String( ".XXXXXX" );

  if ( checkSocketDirectory( link ) )
    return link;

  if ( createSocketDirectory( link, tmpl ) )
    return link;

  qCritical() << "Could not create socket directory for Akonadi.";
  return QString();
}

static bool checkSocketDirectory( const QString &path )
{
  QFileInfo info( path );

  if ( !info.exists() )
    return false;

  if ( info.isSymLink() )
    info = QFileInfo( info.symLinkTarget() );

  if ( !info.isDir() )
    return false;

  if ( info.ownerId() != getuid() )
    return false;

  return true;
}

static bool createSocketDirectory( const QString &link, const QString &tmpl )
{
  QString directory = QString::fromLatin1( "%1%2%3" ).arg( QDir::tempPath() ).arg( QDir::separator() ).arg( tmpl );

  QByteArray directoryString = directory.toLocal8Bit().data();

  if ( !mkdtemp( directoryString.data() ) ) {
    qCritical() << "Creating socket directory with template" << directoryString << "failed:" << strerror( errno );
    return false;
  }

  directory = QString::fromLocal8Bit( directoryString );

  QFile::remove( link );

  if ( !QFile::link( directory, link ) ) {
    qCritical() << "Creating symlink from" << directory << "to" << link << "failed";
    return false;
  }

  return true;
}
