/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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

#include "akonadi.h"
#include "akonadifull-version.h"
#include "akonadiserver_debug.h"

#include <shared/akapplication.h>

#include <private/dbus_p.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QTimer>

#include <KAboutData>

#include <cstdlib>

#ifdef QT_STATICPLUGIN

Q_IMPORT_PLUGIN(qsqlite3)
#endif

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(akonadidb);
    AkCoreApplication app(argc, argv, AKONADISERVER_LOG());
    app.setDescription(QStringLiteral("Akonadi Server\nDo not run manually, use 'akonadictl' instead to start/stop Akonadi."));

    // Set KAboutData so that DrKonqi can report bugs
    KAboutData aboutData(QStringLiteral("akonadiserver"),
                         QStringLiteral("Akonadi Server"), // we don't have any localization in the server
                         QStringLiteral(AKONADI_FULL_VERSION),
                         QStringLiteral("Akonadi Server"), // we don't have any localization in the server
                         KAboutLicense::LGPL_V2);
    KAboutData::setApplicationData(aboutData);

#if !defined(NDEBUG)
    const QCommandLineOption startWithoutControlOption(
        QStringLiteral("start-without-control"),
        QStringLiteral("Allow to start the Akonadi server without the Akonadi control process being available"));
    app.addCommandLineOptions(startWithoutControlOption);
#endif

    app.parseCommandLine();

#if !defined(NDEBUG)
    if (!app.commandLineArguments().isSet(QStringLiteral("start-without-control")) &&
#else
    if (true &&
#endif
            !QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock))) {
        qCCritical(AKONADISERVER_LOG) << "Akonadi control process not found - aborting.";
        qCCritical(AKONADISERVER_LOG) << "If you started akonadiserver manually, try 'akonadictl start' instead.";
    }

    Akonadi::Server::AkonadiServer akonadi;
    // Make sure we do initialization from eventloop, otherwise
    // org.freedesktop.Akonadi.upgrading service won't be registered to DBus at all
    QTimer::singleShot(0, &akonadi, &Akonadi::Server::AkonadiServer::init);

    const int result = app.exec();

    qCInfo(AKONADISERVER_LOG) << "Shutting down AkonadiServer...";
    akonadi.quit();

    Q_CLEANUP_RESOURCE(akonadidb);

    return result;
}
