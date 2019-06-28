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
#include "akonadicontrol_debug.h"
#include "akonadi_version.h"

#include <shared/akapplication.h>

#include <private/dbus_p.h>

#include <QGuiApplication>
#include <QSessionManager>
#include <QDBusConnection>

#include <KCrash/KCrash>
#include <KAboutData>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

static AgentManager *sAgentManager = nullptr;

void crashHandler(int)
{
    if (sAgentManager) {
        sAgentManager->cleanup();
    }

    exit(255);
}

int main(int argc, char **argv)
{
    AkUniqueGuiApplication app(argc, argv, Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock), AKONADICONTROL_LOG());
    app.setDescription(QStringLiteral("Akonadi Control Process\nDo not run this manually, use 'akonadictl' instead to start/stop Akonadi."));

    KAboutData aboutData(QStringLiteral("akonadi_control"),
                         QStringLiteral("Akonadi Control"),
                         QStringLiteral(AKONADI_VERSION_STRING),
                         QStringLiteral("Akonadi Control"),
                         KAboutLicense::LGPL_V2);
    KAboutData::setApplicationData(aboutData);

    app.parseCommandLine();

    // older Akonadi server versions don't use the lock service yet, so check if one is already running before we try to start another one
    // TODO: Remove this legacy check?
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Control))) {
        qCWarning(AKONADICONTROL_LOG) << "Another Akonadi control process is already running.";
        return -1;
    }

    ControlManager controlManager;

    AgentManager agentManager(app.commandLineArguments().isSet(QStringLiteral("verbose")));
    KCrash::setEmergencySaveFunction(crashHandler);

    QGuiApplication::setFallbackSessionManagementEnabled(false);

    // akonadi_control is started on-demand, no need to auto restart by session.
    auto disableSessionManagement = [](QSessionManager & sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(qApp, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(qApp, &QGuiApplication::saveStateRequest, disableSessionManagement);

    return app.exec();
}
