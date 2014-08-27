/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#ifndef AKDBUS_H
#define AKDBUS_H

class QString;

/**
 * Helper methods for obtaining D-Bus identifiers.
 * This should be used instead of hardcoded identifiers or constants to support multi-instance namespacing
 * @since 1.7
 */
namespace AkDBus {
/** D-Bus service types used by the Akonadi server processes. */
enum ServiceType {
    Server,
    Control,
    ControlLock,
    AgentServer,
    StorageJanitor,
    UpgradeIndicator
};

/**
 * Returns the service name for the given @p serviceType.
 */
QString serviceName(ServiceType serviceType);

/** Known D-Bus service name types for agents. */
enum AgentType {
    Unknown,
    Agent,
    Resource,
    Preprocessor
};

/**
 * Parses a D-Bus service name and checks if it belongs to an agent of this instance.
 * @param serviceName The service name to parse.
 * @param agentType Output parameter containing the agent type.
 * @return The identifier of the agent, empty string if that's not an agent (or an agent of a different Akonadi instance)
 */
QString parseAgentServiceName(const QString &serviceName, AkDBus::AgentType &agentType);

/**
 * Returns the D-Bus service name of the agent @p agentIdentifier for type @p agentType.
 */
QString agentServiceName(const QString &agentIdentifier, AkDBus::AgentType agentType);
}

#endif
