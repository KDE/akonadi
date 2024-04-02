/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "processcontrol.h"
#include "akonadictl_debug.h"
#include "controlmanagerinterface.h"

#include "shared/akapplication.h"

#include "private/dbus_p.h"
#include "private/instance_p.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include <iostream>

ProcessControl::ProcessControl(QObject *parent)
    : QObject(parent)
    , mWatcher(Akonadi::DBus::serviceName(Akonadi::DBus::ControlLock), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration)
{
    connect(&mWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this]() {
        mRegistered = true;
        QCoreApplication::instance()->quit();
    });
}

bool ProcessControl::start(bool verbose)
{
    qCInfo(AKONADICTL_LOG) << "Starting Akonadi Server...";

    QStringList serverArgs;
    if (Akonadi::Instance::hasIdentifier()) {
        serverArgs << QStringLiteral("--instance") << Akonadi::Instance::identifier();
    }
    if (verbose) {
        serverArgs << QStringLiteral("--verbose");
    }

    const QString exec = QStandardPaths::findExecutable(QStringLiteral("akonadi_control"));
    if (exec.isEmpty() || !QProcess::startDetached(exec, serverArgs)) {
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

bool ProcessControl::stop()
{
    org::freedesktop::Akonadi::ControlManager iface(Akonadi::DBus::serviceName(Akonadi::DBus::Control),
                                                    QStringLiteral("/ControlManager"),
                                                    QDBusConnection::sessionBus(),
                                                    nullptr);
    if (!iface.isValid()) {
        std::cerr << "Akonadi is not running." << std::endl;
        return false;
    }

    iface.shutdown();

    return true;
}

#include "moc_ProcessControl.cpp"
