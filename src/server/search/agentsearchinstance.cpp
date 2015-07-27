/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "agentsearchinstance.h"
#include "agentsearchinterface.h"
#include "searchtaskmanager.h"
#include "dbusconnectionpool.h"

#include <shared/akdbus.h>

using namespace Akonadi::Server;

AgentSearchInstance::AgentSearchInstance(const QString &id)
    : mId(id)
    , mInterface(0)
    , mServiceWatcher(0)
{
}

AgentSearchInstance::~AgentSearchInstance()
{
    delete mInterface;
}

bool AgentSearchInstance::init()
{
    Q_ASSERT(!mInterface);

    mInterface = new OrgFreedesktopAkonadiAgentSearchInterface(
        AkDBus::agentServiceName(mId, AkDBus::Agent),
        QStringLiteral("/Search"),
        DBusConnectionPool::threadConnection());

    if (!mInterface || !mInterface->isValid()) {
        delete mInterface;
        mInterface = 0;
        return false;
    }

    mServiceWatcher = new QDBusServiceWatcher(AkDBus::agentServiceName(mId, AkDBus::Agent),
                                              DBusConnectionPool::threadConnection(),
                                              QDBusServiceWatcher::WatchForOwnerChange,
                                              this);
    connect(mServiceWatcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    return true;
}

void AgentSearchInstance::serviceOwnerChanged(const QString &service, const QString &oldName, const QString &newName)
{
    Q_UNUSED(service);
    Q_UNUSED(oldName);

    if (newName.isEmpty()) {
        SearchTaskManager::instance()->unregisterInstance(mId);
    }
}

void AgentSearchInstance::search(const QByteArray &searchId, const QString &query,
                                 qlonglong collectionId)
{
    mInterface->search(searchId, query, collectionId);
}

OrgFreedesktopAkonadiAgentSearchInterface *AgentSearchInstance::interface() const
{
    return mInterface;
}
