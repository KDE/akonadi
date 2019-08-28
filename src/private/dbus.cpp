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

#include "dbus_p.h"
#include "instance_p.h"

#include <QString>
#include <QStringList>

using namespace Akonadi;

#define AKONADI_DBUS_SERVER_SERVICE           "org.freedesktop.Akonadi"
#define AKONADI_DBUS_CONTROL_SERVICE          "org.freedesktop.Akonadi.Control"
#define AKONADI_DBUS_CONTROL_SERVICE_LOCK     "org.freedesktop.Akonadi.Control.lock"
#define AKONADI_DBUS_AGENTSERVER_SERVICE      "org.freedesktop.Akonadi.AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_SERVICE   "org.freedesktop.Akonadi.Janitor"
#define AKONADI_DBUS_SERVER_SERVICE_UPGRADING "org.freedesktop.Akonadi.upgrading"

static QString makeServiceName(const char *base)
{
    if (!Instance::hasIdentifier()) {
        return QLatin1String(base);
    }
    return QLatin1String(base) % QLatin1String(".") % Instance::identifier();
}

QString DBus::serviceName(DBus::ServiceType serviceType)
{
    switch (serviceType) {
    case Server:
        return makeServiceName(AKONADI_DBUS_SERVER_SERVICE);
    case Control:
        return makeServiceName(AKONADI_DBUS_CONTROL_SERVICE);
    case ControlLock:
        return makeServiceName(AKONADI_DBUS_CONTROL_SERVICE_LOCK);
    case AgentServer:
        return makeServiceName(AKONADI_DBUS_AGENTSERVER_SERVICE);
    case StorageJanitor:
        return makeServiceName(AKONADI_DBUS_STORAGEJANITOR_SERVICE);
    case UpgradeIndicator:
        return makeServiceName(AKONADI_DBUS_SERVER_SERVICE_UPGRADING);
    }
    Q_ASSERT(!"WTF?");
    return QString();
}

akOptional<DBus::AgentService> DBus::parseAgentServiceName(const QString &serviceName)
{
    if (!serviceName.startsWith(QLatin1String("org.freedesktop.Akonadi."))) {
        return nullopt;
    }
    const QStringList parts = serviceName.mid(24).split(QLatin1Char('.'));
    if ((parts.size() == 2 && !Akonadi::Instance::hasIdentifier())
            || (parts.size() == 3 && Akonadi::Instance::hasIdentifier() && Akonadi::Instance::identifier() == parts.at(2))) {
        // switch on parts.at( 0 )
        const QString &partFirst = parts.constFirst();
        if (partFirst == QLatin1String("Agent")) {
            return AgentService{parts.at(1), DBus::Agent};
        } else if (partFirst == QLatin1String("Resource")) {
            return AgentService{parts.at(1), DBus::Resource};
        } else if (partFirst == QLatin1String("Preprocessor")) {
            return AgentService{parts.at(1), DBus::Preprocessor};
        } else {
            return nullopt;
        }
    }

    return nullopt;
}

QString DBus::agentServiceName(const QString &agentIdentifier, DBus::AgentType agentType)
{
    Q_ASSERT(!agentIdentifier.isEmpty());
    Q_ASSERT(agentType != Unknown);
    QString serviceName = QStringLiteral("org.freedesktop.Akonadi.");
    switch (agentType) {
    case Agent:
        serviceName += QLatin1String("Agent.");
        break;
    case Resource:
        serviceName += QLatin1String("Resource.");
        break;
    case Preprocessor:
        serviceName += QLatin1String("Preprocessor.");
        break;
    default:
        Q_ASSERT(!"WTF?");
    }
    serviceName += agentIdentifier;
    if (Akonadi::Instance::hasIdentifier()) {
        serviceName += QLatin1Char('.') % Akonadi::Instance::identifier();
    }
    return serviceName;
}
