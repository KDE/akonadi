/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "agentprocessinstance.h"

#include "agenttype.h"
#include "akonadicontrol_debug.h"
#include "private/standarddirs_p.h"
#include "processcontrol.h"

#include <QStandardPaths>

using namespace AkonadiControl;

AgentProcessInstance::AgentProcessInstance(AgentManager &manager)
    : AgentInstance(manager)
{
}

bool AgentProcessInstance::start(const AgentType &agentInfo)
{
    Q_ASSERT(!identifier().isEmpty());
    if (identifier().isEmpty()) {
        return false;
    }

    setAgentType(agentInfo.identifier);

    Q_ASSERT(agentInfo.launchMethod == AgentType::Process || agentInfo.launchMethod == AgentType::Launcher);

    const QString executable = (agentInfo.launchMethod == AgentType::Process) ? Akonadi::StandardDirs::findExecutable(agentInfo.exec) : agentInfo.exec;

    if (executable.isEmpty()) {
        qCWarning(AKONADICONTROL_LOG) << "Unable to find agent executable" << agentInfo.exec;
        return false;
    }

    mController = std::make_unique<AkonadiControl::ProcessControl>();
    connect(mController.get(), &ProcessControl::unableToStart, this, &AgentProcessInstance::failedToStart);

    if (agentInfo.launchMethod == AgentType::Process) {
        const QStringList arguments = {QStringLiteral("--identifier"), identifier()};
        mController->start(executable, arguments);
    } else {
        Q_ASSERT(agentInfo.launchMethod == AgentType::Launcher);
        const QStringList arguments = QStringList() << executable << identifier();
        const QString agentLauncherExec = Akonadi::StandardDirs::findExecutable(QStringLiteral("akonadi_agent_launcher"));
        mController->start(agentLauncherExec, arguments);
    }
    return true;
}

void AgentProcessInstance::quit()
{
    mController->setCrashPolicy(AkonadiControl::ProcessControl::StopOnCrash);
    AgentInstance::quit();
}

void AgentProcessInstance::cleanup()
{
    mController->setCrashPolicy(AkonadiControl::ProcessControl::StopOnCrash);
    AgentInstance::cleanup();
}

void AgentProcessInstance::restartWhenIdle()
{
    if (mController->isRunning()) {
        if (status() != 1) {
            mController->restartOnceWhenFinished();
            quit();
        }
    } else {
        mController->start();
    }
}

void AgentProcessInstance::configure(qlonglong windowId)
{
    controlInterface()->configure(windowId);
}

void AgentProcessInstance::failedToStart()
{
    statusChanged(2 /*Broken*/, QStringLiteral("Unable to start."));
}
