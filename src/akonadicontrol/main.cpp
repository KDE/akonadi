/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "agentmanager.h"
#include "controlmanager.h"
#include "processcontrol.h"

#include <shared/akapplication.h>
#include <shared/akcrash.h>
#include <shared/akdebug.h>

#include <private/dbus_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QSessionManager>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

static AgentManager *sAgentManager = 0;

void crashHandler(int)
{
    if (sAgentManager) {
        sAgentManager->cleanup();
    }

    exit(255);
}

int main(int argc, char **argv)
{
    // TODO: use proper AkXXXApplication here
    // QGuiApplication is used to make akonadi_control managed by session so it ends on logout.
    AkApplicationImpl<QGuiApplication> app(argc, argv);
    app.setDescription(QStringLiteral("Akonadi Control Process\nDo not run this manually, use 'akonadictl' instead to start/stop Akonadi."));
    app.parseCommandLine();

    // try to acquire the lock first, that means there is no second instance trying to start up at the same time
    // registering the real service name happens in AgentManager::continueStartup(), when everything is in fact up and running
    if (!QDBusConnection::sessionBus().registerService(Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock))) {
        // We couldn't register. Most likely, it's already running.
        const QString lastError = QDBusConnection::sessionBus().lastError().message();
        if (lastError.isEmpty()) {
            akError() << "Unable to register service as" << Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock) << "Maybe it's already running?";
        } else {
            akError() << "Unable to register service as" << Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock) << "Error was:" << lastError;
        }
        return -1;
    }

    // older Akonadi server versions don't use the lock service yet, so check if one is already running before we try to start another one
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Control))) {
        akError() << "Another Akonadi control process is already running.";
        return -1;
    }

    new ControlManager;

    sAgentManager = new AgentManager;
    AkonadiCrash::setEmergencyMethod(crashHandler);

    // akonadi_control is started on-demand, no need to auto restart by session.
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(qApp, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(qApp, &QGuiApplication::saveStateRequest, disableSessionManagement);

    int retval = app.exec();

    delete sAgentManager;
    sAgentManager = 0;

    return retval;
}
