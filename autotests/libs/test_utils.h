/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_TEST_UTILS_H
#define AKONADI_TEST_UTILS_H

#include "collectionpathresolver.h"
#include "servermanager.h"
#include "qtest_akonadi.h"
#include "monitor.h"
#include "collectionfetchscope.h"
#include "itemfetchscope.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

qint64 collectionIdFromPath(const QString &path)
{
    Akonadi::CollectionPathResolver *resolver = new Akonadi::CollectionPathResolver(path);
    bool success = resolver->exec();
    if (!success) {
        qDebug() << "path resolution for " << path << " failed: " << resolver->errorText();
        return -1;
    }
    qint64 id = resolver->collection();
    return id;
}

QString testrunnerServiceName()
{
    const QString pid = QString::fromLocal8Bit(qgetenv("AKONADI_TESTRUNNER_PID"));
    Q_ASSERT(!pid.isEmpty());
    return QStringLiteral("org.kde.Akonadi.Testrunner-") + pid;
}

bool restartAkonadiServer()
{
    QDBusInterface testrunnerIface(testrunnerServiceName(),
                                   QStringLiteral("/"),
                                   QStringLiteral("org.kde.Akonadi.Testrunner"),
                                   QDBusConnection::sessionBus());
    if (!testrunnerIface.isValid()) {
        qWarning() << "Unable to get a dbus interface to the testrunner!";
    }

    QDBusReply<void> reply = testrunnerIface.call(QStringLiteral("restartAkonadiServer"));
    if (!reply.isValid()) {
        qWarning() << reply.error();
        return false;
    } else if (Akonadi::ServerManager::isRunning()) {
        return true;
    } else {
        bool ok = false;
        [&]() {
            QSignalSpy spy(Akonadi::ServerManager::self(), &Akonadi::ServerManager::started);
            QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 10000);
            ok = true;
        }();
        return ok;
    }
}

bool trackAkonadiProcess(bool track)
{
    QDBusInterface testrunnerIface(testrunnerServiceName(),
                                   QStringLiteral("/"),
                                   QStringLiteral("org.kde.Akonadi.Testrunner"),
                                   QDBusConnection::sessionBus());
    if (!testrunnerIface.isValid()) {
        qWarning() << "Unable to get a dbus interface to the testrunner!";
    }

    QDBusReply<void> reply = testrunnerIface.call(QStringLiteral("trackAkonadiProcess"), track);
    if (!reply.isValid()) {
        qWarning() << reply.error();
        return false;
    } else {
        return true;
    }
}

std::unique_ptr<Akonadi::Monitor> getTestMonitor()
{
    auto m = new Akonadi::Monitor();
    m->fetchCollection(true);
    m->setCollectionMonitored(Akonadi::Collection::root(), true);
    m->setAllMonitored(true);
    auto &itemFS = m->itemFetchScope();
    itemFS.setAncestorRetrieval(Akonadi::ItemFetchScope::All);
    auto &colFS = m->collectionFetchScope();
    colFS.setAncestorRetrieval(Akonadi::CollectionFetchScope::All);

    QSignalSpy readySpy(m, &Akonadi::Monitor::monitorReady);
    readySpy.wait();

    return std::unique_ptr<Akonadi::Monitor>(m);
}

#endif
