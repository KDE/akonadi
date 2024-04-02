/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentsystemdinstance.h"
#include "agentmanager.h"
#include "agenttype.h"
#include "akonadicontrol_debug.h"
#include "shared/systemd.h"
#include "systemd_manager.h"

#include <QDBusPendingReply>

using namespace Akonadi;

AgentSystemdInstance::AgentSystemdInstance(AgentManager &manager)
    : AgentInstance(manager)
{
    Systemd::registerDBusTypes();
}

void AgentSystemdInstance::cleanup()
{
    AgentInstance::cleanup();

    org::freedesktop::systemd1::Manager manager(Systemd::DBusService, Systemd::DBusPath, QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to connect to systemd manager";
        return;
    }

    const auto unitFile = this->unitFile(mAgentType);

    if (const auto reply = manager.DisableUnitFiles({unitFile}, false); reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to disable unit file" << unitFile << reply.error().message();
        return;
    } else {
        qCInfo(AKONADICONTROL_LOG) << "Disabled unit file" << unitFile;
    }

    if (const auto reply = manager.StopUnit(unitFile, QStringLiteral("fail")); reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to stop unit file" << unitFile << reply.error().message();
        return;
    } else {
        qCInfo(AKONADICONTROL_LOG) << "Stopped unit file" << unitFile;
    }
}

bool AgentSystemdInstance::start(const AgentType &agentInfo)
{
    mAgentType = agentInfo;
    auto unitFile = this->unitFile(agentInfo);

    setAgentType(agentInfo.identifier);

    org::freedesktop::systemd1::Manager manager(Systemd::DBusService, Systemd::DBusPath, QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to connect to systemd manager";
        return false;
    }

    if (const auto reply = manager.GetUnitFileState(unitFile); reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to get unit file state" << unitFile << reply.error().message();
        return false;
    } else if (reply.value() == u"enabled") {
        // If already enabled, nothing for us to do, systemd will start it once akonadicontrol is finished.
        return true;
    }

    if (const auto reply = manager.EnableUnitFiles({unitFile}, false, false); reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to enable unit file" << unitFile << reply.error().message();
        return false;
    } else {
        qCInfo(AKONADICONTROL_LOG) << "Enabled unit file" << unitFile;
    }

    // We have to start it manually, since enable only enables it, but it wouldn't get auto-started by systemd
    if (const auto reply = manager.StartUnit(unitFile, QStringLiteral("fail")); reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to start unit file" << unitFile << reply.error().message();
        return false;
    } else {
        qCInfo(AKONADICONTROL_LOG) << "Started unit file" << unitFile;
    }

    return true;
}

void AgentSystemdInstance::quit()
{
    // no-op, managed by systemd
}

void AgentSystemdInstance::restartWhenIdle()
{
    org::freedesktop::systemd1::Manager manager(Systemd::DBusService, Systemd::DBusPath, QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to connect to systemd manager";
        return;
    }

    const auto unitFile = this->unitFile(mAgentType);

    manager.ResetFailedUnit(unitFile);

    if (const auto reply = manager.RestartUnit(unitFile, QStringLiteral("replace")); reply.isError()) {
        qCWarning(AKONADICONTROL_LOG) << "Failed to get unit file state" << unitFile << reply.error().message();
        return;
    } else {
        qCInfo(AKONADICONTROL_LOG) << "Restarted unit file" << unitFile;
    }
}

void AgentSystemdInstance::configure(qlonglong windowId)
{
    controlInterface()->configure(windowId);
}

QString AgentSystemdInstance::unitFile(const AgentType &agentType) const
{
    const bool isUnique = agentType.capabilities.contains(AgentType::CapabilityUnique) || !agentType.capabilities.contains(AgentType::CapabilityResource);
    QString unitFile = identifier();
    if (!isUnique) {
        // Take the identifier, which is something like "akonadi_foobar_resource_0" and turn it to "akonadi_foobar_resource@0.service"
        const auto pos = unitFile.lastIndexOf(QLatin1Char('_'));
        Q_ASSERT(pos > 0);
        unitFile[pos] = QLatin1Char('@');
    }

    return QStringLiteral("%1.service").arg(unitFile);
}