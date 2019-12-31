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

#include <private/dbus_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

AgentSearchInstance::AgentSearchInstance(const QString &id)
    : mId(id)
    , mInterface(nullptr)
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
        DBus::agentServiceName(mId, DBus::Agent),
        QStringLiteral("/Search"),
        DBusConnectionPool::threadConnection());

    if (!mInterface || !mInterface->isValid()) {
        delete mInterface;
        mInterface = nullptr;
        return false;
    }

    mServiceWatcher = std::make_unique<QDBusServiceWatcher>(
            DBus::agentServiceName(mId, DBus::Agent), DBusConnectionPool::threadConnection(),
            QDBusServiceWatcher::WatchForUnregistration);
    connect(mServiceWatcher.get(), &QDBusServiceWatcher::serviceUnregistered,
            this, [this]() {
                SearchTaskManager::instance()->unregisterInstance(mId);
            });

    return true;
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
