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
#include "akapplication.h"
#include "akdbus.h"
#include "akdebug.h"
#include "akcrash.h"

#include "protocol_p.h"

#include <QtCore/QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QTimer>

#include <cstdlib>

#ifdef QT_STATICPLUGIN
#include <QtPlugin>

Q_IMPORT_PLUGIN(qsqlite3)
#endif

namespace po = boost::program_options;

void shutdownHandler(int)
{
    akDebug() << "Shutting down AkonadiServer...";

    Akonadi::Server::AkonadiServer::instance()->quit();

    exit(255);
}

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(akonadidb);
    AkCoreApplication app(argc, argv);
    app.setDescription(QLatin1String("Akonadi Server\nDo not run manually, use 'akonadictl' instead to start/stop Akonadi."));

#if !defined(NDEBUG)
    po::options_description debugOptions("Debug options (use with care)");
    debugOptions.add_options()
    ("start-without-control", "Allow to start the Akonadi server even without the Akonadi control process being available");
    app.addCommandLineOptions(debugOptions);
#endif

    app.parseCommandLine();

    if (!app.commandLineArguments().count("start-without-control") &&
        !QDBusConnection::sessionBus().interface()->isServiceRegistered(AkDBus::serviceName(AkDBus::ControlLock))) {
        akError() << "Akonadi control process not found - aborting.";
        akFatal() << "If you started akonadiserver manually, try 'akonadictl start' instead.";
    }

    // Make sure we do initialization from eventloop, otherwise
    // org.freedesktop.Akonadi.upgrading service won't be registered to DBus at all
    QTimer::singleShot(0, Akonadi::Server::AkonadiServer::instance(), SLOT(init()));
    AkonadiCrash::setShutdownMethod(shutdownHandler);

    const int result = app.exec();

    Akonadi::Server::AkonadiServer::instance()->quit();

    Q_CLEANUP_RESOURCE(akonadidb);

    return result;
}
