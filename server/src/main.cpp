/***************************************************************************
 *   Copyright (C) 2006 by Till Adam   *
 *   adam@kde.org   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include "akonadi.h"
#include "akdebug.h"
#include "akcrash.h"

#include <cstdlib>

void shutdownHandler( int )
{
  qDebug( "Shutting down AkonadiServer..." );

  Akonadi::AkonadiServer::instance()->quit();

  exit( 255 );
}

int main( int argc, char ** argv )
{
    akInit( QString::fromLatin1( "akonadiserver" ) );
    AkonadiCrash::setShutdownMethod( shutdownHandler );

    QCoreApplication app( argc, argv );

    Akonadi::AkonadiServer::instance(); // trigger singleton creation

    if ( !QDBusConnection::sessionBus().registerService( QLatin1String("org.freedesktop.Akonadi") ) ) {
      qDebug( "Unable to connect to dbus service: %s", qPrintable( QDBusConnection::sessionBus().lastError().message() ) );
      return 1;
    }

    const int result = app.exec();

    Akonadi::AkonadiServer::instance()->quit();

    return result;
}
