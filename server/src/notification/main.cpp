/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <QCoreApplication>
#include <dbus/qdbus.h>
#include "notificationmanager.h"
#include <stdlib.h>

int main( int argc, char ** argv ) {
    QCoreApplication a( argc, argv );
    QDBusBusService *bus = QDBus::sessionBus().busService();
    if ( bus->requestName("org.kde.pim.Akonadi.NotificationManager", QDBusBusService::AllowReplacingName ) !=
        QDBusBusService::PrimaryOwnerReply)
      exit(1);
    Akonadi::NotificationManager *nm = new Akonadi::NotificationManager();
    QDBus::sessionBus().registerObject("/", nm, QDBusConnection::ExportAdaptors );
    return a.exec();
    delete nm;
}
