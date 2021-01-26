/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadistarter.h"
#include "akonadictl_debug.h"

#include <shared/akapplication.h>

#include <private/dbus_p.h>
#include <private/instance_p.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QProcess>
#include <QTimer>

#include <iostream>

AkonadiStarter::AkonadiStarter(QObject *parent)
    : QObject(parent)
    , mWatcher(Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration)
{
    connect(&mWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this]() {
        mRegistered = true;
        QCoreApplication::instance()->quit();
    });
}

bool AkonadiStarter::start(bool verbose)
{
    qCInfo(AKONADICTL_LOG) << "Starting Akonadi Server...";

    QStringList serverArgs;
    if (Akonadi::Instance::hasIdentifier()) {
        serverArgs << QStringLiteral("--instance") << Akonadi::Instance::identifier();
    }
    if (verbose) {
        serverArgs << QStringLiteral("--verbose");
    }

    const bool ok = QProcess::startDetached(QStringLiteral("akonadi_control"), serverArgs);
    if (!ok) {
        std::cerr << "Error: unable to execute binary akonadi_control" << std::endl;
        return false;
    }

    // safety timeout
    QTimer::singleShot(std::chrono::seconds{5}, QCoreApplication::instance(), &QCoreApplication::quit);
    // wait for the server to register with D-Bus
    QCoreApplication::instance()->exec();

    if (!mRegistered) {
        std::cerr << "Error: akonadi_control was started but didn't register at D-Bus session bus." << std::endl
                  << "Make sure your system is set up correctly!" << std::endl;
        return false;
    }

    qCInfo(AKONADICTL_LOG) << "   done.";
    return true;
}
