/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiprivate_export.h"

#include <QString>

#include <optional>

/**
 * Helper methods for obtaining D-Bus identifiers.
 * This should be used instead of hardcoded identifiers or constants to support multi-instance namespacing
 * @since 1.7
 */

#define AKONADI_DBUS_AGENTMANAGER_PATH "/AgentManager"
#define AKONADI_DBUS_AGENTSERVER_PATH "/AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_PATH "/Janitor"

namespace Akonadi
{
namespace DBus
{
/** D-Bus service types used by the Akonadi server processes. */
enum ServiceType {
    Server,
    Control,
    ControlLock,
    AgentServer,
    StorageJanitor,
    UpgradeIndicator,
};

/**
 * Returns the service name for the given @p serviceType.
 */
AKONADIPRIVATE_EXPORT QString serviceName(ServiceType serviceType);

/** Known D-Bus service name types for agents. */
enum AgentType {
    Unknown,
    Agent,
    Resource,
    Preprocessor,
};

struct AgentService {
    QString identifier{};
    DBus::AgentType agentType{DBus::Unknown};
};

/**
 * Parses a D-Bus service name and checks if it belongs to an agent of this instance.
 * @param serviceName The service name to parse.
 * @return The identifier of the agent, empty string if that's not an agent (or an agent of a different Akonadi instance)
 */
AKONADIPRIVATE_EXPORT std::optional<AgentService> parseAgentServiceName(const QString &serviceName);

/**
 * Returns the D-Bus service name of the agent @p agentIdentifier for type @p agentType.
 */
AKONADIPRIVATE_EXPORT QString agentServiceName(const QString &agentIdentifier, DBus::AgentType agentType);

/**
 * Returns the Akonadi instance name encoded in the service name.
 */
AKONADIPRIVATE_EXPORT std::optional<QString> parseInstanceIdentifier(const QString &serviceName);

}

}

