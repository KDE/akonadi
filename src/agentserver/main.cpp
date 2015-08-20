/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "agentserver.h"

#include <private/dbus_p.h>

#include <shared/akapplication.h>
#include <shared/akdebug.h>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QApplication>

int main(int argc, char **argv)
{
    AkGuiApplication app(argc, argv);
    app.setDescription(QStringLiteral("Akonadi Agent Server\nDo not run manually, use 'akonadictl' instead to start/stop Akonadi."));
    app.parseCommandLine();
    qApp->setQuitOnLastWindowClosed(false);

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock))) {
        akError() << "Akonadi control process not found - aborting.";
        akFatal() << "If you started akonadi_agent_server manually, try 'akonadictl start' instead.";
    }

    new Akonadi::AgentServer(&app);

    if (!QDBusConnection::sessionBus().registerService(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer))) {
        akFatal() << "Unable to connect to dbus service: " << QDBusConnection::sessionBus().lastError().message();
    }

    return app.exec();
}
