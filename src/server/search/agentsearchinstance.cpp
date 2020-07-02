/*
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentsearchinstance.h"
#include "agentsearchinterface.h"
#include "searchtaskmanager.h"

#include <private/dbus_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

AgentSearchInstance::AgentSearchInstance(const QString &id, SearchTaskManager &manager)
    : mId(id)
    , mInterface(nullptr)
    , mManager(manager)
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
        QDBusConnection::sessionBus());

    if (!mInterface || !mInterface->isValid()) {
        delete mInterface;
        mInterface = nullptr;
        return false;
    }

    mServiceWatcher = std::make_unique<QDBusServiceWatcher>(
            DBus::agentServiceName(mId, DBus::Agent), QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForUnregistration);
    connect(mServiceWatcher.get(), &QDBusServiceWatcher::serviceUnregistered,
            this, [this]() {
                mManager.unregisterInstance(mId);
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
