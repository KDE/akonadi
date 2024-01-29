/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentsearchengine.h"
#include "akonadiserver_search_debug.h"
#include "entities.h"

#include <private/dbus_p.h>

#include <QDBusInterface>

using namespace Akonadi;
using namespace Akonadi::Server;

void AgentSearchEngine::addSearch(const Collection &collection)
{
    QDBusInterface agentMgr(DBus::serviceName(DBus::Control),
                            QStringLiteral(AKONADI_DBUS_AGENTMANAGER_PATH),
                            QStringLiteral("org.freedesktop.Akonadi.AgentManagerInternal"));
    if (agentMgr.isValid()) {
        const QList<QVariant> args = QList<QVariant>() << collection.queryString() << QLatin1StringView("") << collection.id();
        agentMgr.callWithArgumentList(QDBus::NoBlock, QStringLiteral("addSearch"), args);
        return;
    }

    qCCritical(AKONADISERVER_SEARCH_LOG) << "Failed to connect to agent manager: " << agentMgr.lastError().message();
}

void AgentSearchEngine::removeSearch(qint64 id)
{
    QDBusInterface agentMgr(DBus::serviceName(DBus::Control),
                            QStringLiteral(AKONADI_DBUS_AGENTMANAGER_PATH),
                            QStringLiteral("org.freedesktop.Akonadi.AgentManagerInternal"));
    if (agentMgr.isValid()) {
        const QList<QVariant> args = {id};
        agentMgr.callWithArgumentList(QDBus::NoBlock, QStringLiteral("removeSearch"), args);
        return;
    }

    qCCritical(AKONADISERVER_SEARCH_LOG) << "Failed to connect to agent manager: " << agentMgr.lastError().message();
}
