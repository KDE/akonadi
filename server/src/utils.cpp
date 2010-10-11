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

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

using namespace Akonadi;

QString Utils::preferredSocketDirectory( const QString &defaultDirectory )
{
  const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
  const QSettings serverSettings( serverConfigFile, QSettings::IniFormat );

  QString socketDir = serverSettings.value( QLatin1String( "Connection/SocketDirectory" ), defaultDirectory ).toString();

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
