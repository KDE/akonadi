/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "akonadistarter.h"

#include <akapplication.h>
#include <akdbus.h>
#include <akdebug.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusServiceWatcher>

AkonadiStarter::AkonadiStarter(QObject *parent)
    : QObject(parent)
    , mRegistered(false)
{
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(AkDBus::serviceName(AkDBus::ControlLock),
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForOwnerChange, this);

    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));
}

bool AkonadiStarter::start()
{
    akDebug() << "Starting Akonadi Server...";

    QStringList serverArgs;
    if (AkApplication::hasInstanceIdentifier()) {
        serverArgs << QStringLiteral("--instance") << AkApplication::instanceIdentifier();
    }

    const bool ok = QProcess::startDetached(QStringLiteral("akonadi_control"), serverArgs);
    if (!ok) {
        akError() << "Error: unable to execute binary akonadi_control";
        return false;
    }

    // safety timeout
    QTimer::singleShot(5000, QCoreApplication::instance(), SLOT(quit()));
    // wait for the server to register with D-Bus
    QCoreApplication::instance()->exec();

    if (!mRegistered) {
        akError() << "Error: akonadi_control was started but didn't register at D-Bus session bus.";
        akError() << "Make sure your system is set up correctly!";
        return false;
    }

    akDebug() << "   done.";
    return true;
}

void AkonadiStarter::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(name);
    Q_UNUSED(oldOwner);
    if (newOwner.isEmpty()) {
        return;
    }

    mRegistered = true;
    QCoreApplication::instance()->quit();
}
