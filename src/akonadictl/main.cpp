/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
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

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <shared/akapplication.h>


#include "controlmanagerinterface.h"
#include "janitorinterface.h"
#include "akonadistarter.h"
#include <QSettings>

#include <private/protocol_p.h>
#include <private/xdgbasedirs_p.h>
#include <private/dbus_p.h>

#if defined(HAVE_UNISTD_H) && !defined(Q_WS_WIN)
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <iostream>

static bool startServer(bool verbose)
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Control))
        || QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Server))) {
        std::cerr << "Akonadi is already running." << std::endl;
        return false;
    }
    AkonadiStarter starter;
    return starter.start(verbose);
}

static bool stopServer()
{
    org::freedesktop::Akonadi::ControlManager iface(Akonadi::DBus::serviceName(Akonadi::DBus::Control),
                                                    QStringLiteral("/ControlManager"),
                                                    QDBusConnection::sessionBus(), nullptr);
    if (!iface.isValid()) {
        std::cerr << "Akonadi is not running." << std::endl;
        return false;
    }

    iface.shutdown();

    return true;
}

static bool checkAkonadiControlStatus()
{
    const bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Control));
    std::cerr << "Akonadi Control: " << (registered ? "running" : "stopped") << std::endl;
    return registered;
}

static bool checkAkonadiServerStatus()
{
    const bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Server));
    std::cerr << "Akonadi Server: " << (registered ? "running" : "stopped") << std::endl;
    return registered;
}

static bool checkSearchSupportStatus()
{
    QStringList searchMethods;
    searchMethods << QStringLiteral("Remote Search");

    const QString pluginOverride = QString::fromLatin1(qgetenv("AKONADI_OVERRIDE_SEARCHPLUGIN"));
    if (!pluginOverride.isEmpty()) {
        searchMethods << pluginOverride;
    } else {
        const QStringList dirs = Akonadi::XdgBaseDirs::findPluginDirs();
        for (const QString &pluginDir : dirs) {
            QDir dir(pluginDir + QLatin1String("/akonadi"));
            const QStringList desktopFiles = dir.entryList(QStringList() << QStringLiteral("*.desktop"), QDir::Files);
            for (const QString &desktopFileName : desktopFiles) {
                QSettings desktop(pluginDir + QLatin1String("/akonadi/") + desktopFileName, QSettings::IniFormat);
                desktop.beginGroup(QStringLiteral("Desktop Entry"));
                if (desktop.value(QStringLiteral("Type")).toString() != QLatin1String("AkonadiSearchPlugin")) {
                    continue;
                }
                if (!desktop.value(QStringLiteral("X-Akonadi-LoadByDefault"), true).toBool()) {
                    continue;
                }

                searchMethods << desktop.value(QStringLiteral("Name")).toString();
            }
        }
    }

    // There's always at least server-search available
    std::cerr << "Akonadi Server Search Support: available (" << searchMethods.join(QStringLiteral(", ")).toStdString() << ")" << std::endl;
    return true;
}

static bool checkAvailableAgentTypes()
{
    const QStringList dirs = Akonadi::XdgBaseDirs::findAllResourceDirs("data", QStringLiteral("akonadi/agents"));
    QStringList types;
    for (const QString &pluginDir : dirs) {
        QDir dir(pluginDir);
        const QStringList plugins = dir.entryList(QStringList() << QStringLiteral("*.desktop"), QDir::Files);
        for (const QString &plugin : plugins) {
            QSettings pluginInfo(pluginDir + QLatin1String("/") + plugin, QSettings::IniFormat);
            pluginInfo.beginGroup(QStringLiteral("Desktop Entry"));
            types << pluginInfo.value(QStringLiteral("X-Akonadi-Identifier")).toString();
        }
    }

    // Remove duplicates from multiple pluginDirs
    types.removeDuplicates();
    types.sort();

    std::cerr << "Available Agent Types: ";
    if (types.isEmpty()) {
        std::cerr << "No agent types found!" << std::endl;
    } else {
        std::cerr << types.join(QStringLiteral(", ")).toStdString() << std::endl;
    }

    return true;
}

static bool statusServer()
{
    checkAkonadiControlStatus();
    checkAkonadiServerStatus();
    checkSearchSupportStatus();
    checkAvailableAgentTypes();
    return true;
}

static void runJanitor(const QString &operation)
{
    org::freedesktop::Akonadi::Janitor janitor(Akonadi::DBus::serviceName(Akonadi::DBus::StorageJanitor),
                                               QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH),
                                               QDBusConnection::sessionBus());
    QObject::connect(&janitor, &org::freedesktop::Akonadi::Janitor::information,
                        [](const QString &msg) {
                            std::cerr << msg.toStdString() << std::endl;
                        });
    QObject::connect(&janitor, &org::freedesktop::Akonadi::Janitor::done,
                     []() {
                         qApp->exit();
                     });
    janitor.asyncCall(operation);
    qApp->exec();
}

int main(int argc, char **argv)
{
    AkCoreApplication app(argc, argv);

    app.setDescription(QStringLiteral("Akonadi server manipulation tool\n\n"
                                     "Commands:\n"
                                     "  start          Starts the Akonadi server with all its processes\n"
                                     "  stop           Stops the Akonadi server and all its processes cleanly\n"
                                     "  restart        Restart Akonadi server with all its processes\n"
                                     "  status         Shows a status overview of the Akonadi server\n"
                                     "  vacuum         Vacuum internal storage (WARNING: needs a lot of time and disk\n"
                                     "                 space!)\n"
                                     "  fsck           Check (and attempt to fix) consistency of the internal storage\n"
                                     "                 (can take some time)"));


    app.addPositionalCommandLineOption(QStringLiteral("command"), QStringLiteral("Command to execute"),
                                       QStringLiteral("start|stop|restart|status|vacuum|fsck"));

    app.parseCommandLine();
    const QStringList commands = app.commandLineArguments().positionalArguments();
    if (commands.size() != 1) {
        app.printUsage();
        return -1;
    }
    const bool verbose = app.commandLineArguments().isSet(QStringLiteral("verbose"));

    const QString command = commands[0];
    if (command == QLatin1String("start")) {
        if (!startServer(verbose)) {
            return 3;
        }
    } else if (command == QLatin1String("stop")) {
        if (!stopServer()) {
            return 4;
        }
    } else if (command == QLatin1String("status")) {
        if (!statusServer()) {
            return 5;
        }
    } else if (command == QLatin1String("restart")) {
        if (!stopServer()) {
            return 4;
        } else {
            do {
#if defined(HAVE_UNISTD_H) && !defined(Q_WS_WIN)
                usleep(100000);
#else
                Sleep(100000);
#endif
            } while (QDBusConnection::sessionBus().interface()->isServiceRegistered(Akonadi::DBus::serviceName(Akonadi::DBus::Control)));
            if (!startServer(verbose)) {
                return 3;
            }
        }
    } else if (command == QLatin1String("vacuum")) {
        runJanitor(QStringLiteral("vacuum"));
    } else if (command == QLatin1String("fsck")) {
        runJanitor(QStringLiteral("check"));
    } else {
        app.printUsage();
        return -1;
    }
    return 0;
}
