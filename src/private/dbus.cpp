/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbus_p.h"
#include "instance_p.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <QStringView>

#include <array>

using namespace Akonadi;

#define AKONADI_DBUS_SERVER_SERVICE u"org.freedesktop.Akonadi"
#define AKONADI_DBUS_CONTROL_SERVICE u"org.freedesktop.Akonadi.Control"
#define AKONADI_DBUS_CONTROL_SERVICE_LOCK u"org.freedesktop.Akonadi.Control.lock"
#define AKONADI_DBUS_AGENTSERVER_SERVICE u"org.freedesktop.Akonadi.AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_SERVICE u"org.freedesktop.Akonadi.Janitor"
#define AKONADI_DBUS_SERVER_SERVICE_UPGRADING u"org.freedesktop.Akonadi.upgrading"

static QString makeServiceName(QStringView base)
{
    if (!Instance::hasIdentifier()) {
        return base.toString();
    }
    return base + QLatin1Char('.') + Instance::identifier();
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

std::optional<DBus::AgentService> DBus::parseAgentServiceName(const QString &serviceName)
{
    if (!serviceName.startsWith(AKONADI_DBUS_SERVER_SERVICE ".")) {
        return std::nullopt;
    }
    const auto parts = QStringView(serviceName).mid(QStringView(AKONADI_DBUS_SERVER_SERVICE ".").length()).split(QLatin1Char('.'));
    if ((parts.size() == 2 && !Akonadi::Instance::hasIdentifier())
        || (parts.size() == 3 && Akonadi::Instance::hasIdentifier() && Akonadi::Instance::identifier() == parts.at(2))) {
        // switch on parts.at( 0 )
        if (parts.at(0) == QLatin1StringView("Agent")) {
            return AgentService{parts.at(1).toString(), DBus::Agent};
        } else if (parts.at(0) == QLatin1StringView("Resource")) {
            return AgentService{parts.at(1).toString(), DBus::Resource};
        } else if (parts.at(0) == QLatin1StringView("Preprocessor")) {
            return AgentService{parts.at(1).toString(), DBus::Preprocessor};
        } else {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

QString DBus::agentServiceName(const QString &agentIdentifier, DBus::AgentType agentType)
{
    Q_ASSERT(!agentIdentifier.isEmpty());
    Q_ASSERT(agentType != Unknown);
    QString serviceName = QStringLiteral(AKONADI_DBUS_SERVER_SERVICE ".");
    switch (agentType) {
    case Agent:
        serviceName += QLatin1StringView("Agent.");
        break;
    case Resource:
        serviceName += QLatin1StringView("Resource.");
        break;
    case Preprocessor:
        serviceName += QLatin1StringView("Preprocessor.");
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

std::optional<QString> DBus::parseInstanceIdentifier(const QString &serviceName)
{
    constexpr std::array<QStringView, 5> services = {QStringView{AKONADI_DBUS_STORAGEJANITOR_SERVICE},
                                                     QStringView{AKONADI_DBUS_SERVER_SERVICE_UPGRADING},
                                                     QStringView{AKONADI_DBUS_AGENTSERVER_SERVICE},
                                                     QStringView{AKONADI_DBUS_CONTROL_SERVICE_LOCK},
                                                     QStringView{AKONADI_DBUS_CONTROL_SERVICE}};
    for (const auto &service : services) {
        if (serviceName.startsWith(service)) {
            if (serviceName != service) {
                return serviceName.mid(service.length() + 1); // +1 for the separator "."
            }
            return std::nullopt;
        }
    }

    if (serviceName.startsWith(QStringView{AKONADI_DBUS_SERVER_SERVICE})) {
        const auto split = QStringView(serviceName).split(QLatin1Char('.'));
        if (split.size() <= 3) {
            return std::nullopt;
        }

        // [0]org.[1]freedesktop.[2]Akonadi.[3]type.[4]identifier.[5]instance
        if (split[3] == QStringView{u"Agent"} || split[3] == QStringView{u"Resource"} || split[3] == QStringView{u"Preprocessor"}) {
            if (split.size() == 6) {
                return split[5].toString();
            } else {
                return std::nullopt;
            }
        } else {
            return split[3].toString();
        }
    }

    return std::nullopt;
}
