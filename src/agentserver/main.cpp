/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentserver.h"
#include "akonadiagentserver_debug.h"

#include <private/dbus_p.h>

#include <shared/akapplication.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>

int main(int argc, char **argv)
{
    AkCoreApplication app(argc, argv, AKONADIAGENTSERVER_LOG());
    app.setDescription(QStringLiteral("Akonadi Agent Server\nDo not run manually, use 'akonadictl' instead to start/stop Akonadi."));
    app.parseCommandLine();

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock))) {
        qCCritical(AKONADIAGENTSERVER_LOG) << "Akonadi control process not found - aborting.";
        qFatal("If you started akonadi_agent_server manually, try 'akonadictl start' instead.");
    }

    new Akonadi::AgentServer(&app);

    if (!QDBusConnection::sessionBus().registerService(Akonadi::DBus::serviceName(Akonadi::DBus::AgentServer))) {
        qFatal("Unable to connect to dbus service: %s", qPrintable(QDBusConnection::sessionBus().lastError().message()));
    }

    return app.exec();
}
