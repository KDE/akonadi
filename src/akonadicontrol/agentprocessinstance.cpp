/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>

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
#include "agentprocessinstance.h"

#include "agenttype.h"
#include "processcontrol.h"
#include "akonadicontrol_debug.h"
#include "private/standarddirs_p.h"

#include <QStandardPaths>

using namespace Akonadi;

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

    Q_ASSERT(agentInfo.launchMethod == AgentType::Process ||
             agentInfo.launchMethod == AgentType::Launcher);

    const QString executable = (agentInfo.launchMethod == AgentType::Process)
                               ? Akonadi::StandardDirs::findExecutable(agentInfo.exec) : agentInfo.exec;

    if (executable.isEmpty()) {
        qCWarning(AKONADICONTROL_LOG) << "Unable to find agent executable" << agentInfo.exec;
        return false;
    }

    mController = std::make_unique<Akonadi::ProcessControl>();
    connect(mController.get(), &ProcessControl::unableToStart, this, &AgentProcessInstance::failedToStart);

    if (agentInfo.launchMethod == AgentType::Process) {
        const QStringList arguments = { QStringLiteral("--identifier"), identifier()};
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
    mController->setCrashPolicy(Akonadi::ProcessControl::StopOnCrash);
    AgentInstance::quit();
}

void AgentProcessInstance::cleanup()
{
    mController->setCrashPolicy(Akonadi::ProcessControl::StopOnCrash);
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

void Akonadi::AgentProcessInstance::configure(qlonglong windowId)
{
    controlInterface()->configure(windowId);
}

void AgentProcessInstance::failedToStart()
{
    statusChanged(2 /*Broken*/, QStringLiteral("Unable to start."));
}
