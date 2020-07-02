/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "agentmanager.h"
#include "controlmanager.h"
#include "processcontrol.h"
#include "akonadicontrol_debug.h"
#include "akonadi_version.h"
#include "config-akonadi.h"

#ifdef WITH_ACCOUNTS
#include "accountsintegration.h"
#endif

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

void crashHandler(int /*unused*/)
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
#ifdef WITH_ACCOUNTS
    AccountsIntegration accountsIntegration(agentManager);
#endif
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
