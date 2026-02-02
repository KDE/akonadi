/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "agentmanager.h"
#include "akonadi_version.h"
#include "akonadicontrol_debug.h"
#include "config-akonadi.h"
#include "controlmanager.h"
#include "onlineaccountsintegration.h"

#include "shared/akapplication.h"

#include "private/dbus_p.h"

#include <QDBusConnection>
#include <QGuiApplication>
#include <QSessionManager>

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace Qt::Literals;

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
    // akonadi_control is started on-demand, no need to auto restart by session.
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);

    AkUniqueGuiApplication app(argc, argv, Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock), AKONADICONTROL_LOG());
    app.setDescription(QStringLiteral("Akonadi Control Process\nDo not run this manually, use 'akonadictl' instead to start/stop Akonadi."));

    KAboutData aboutData(QStringLiteral("akonadi_control"),
                         i18n("Akonadi Control"),
                         QStringLiteral(AKONADI_VERSION_STRING),
                         i18n("Akonadi Control"),
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

    std::unique_ptr<OnlineAccountsIntegration> accountsIntegration;

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(u"org.kde.KOnlineAccounts"_s)) {
        accountsIntegration = std::make_unique<OnlineAccountsIntegration>(agentManager);
    }

    KCrash::setEmergencySaveFunction(crashHandler);

    return app.exec();
}
