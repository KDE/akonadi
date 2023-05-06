/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentbrokeninstance.h"

using namespace AkonadiControl;

AgentBrokenInstance::AgentBrokenInstance(const QString &type, AgentManager &manager)
    : AgentInstance(manager)
{
    setAgentType(type);
    statusChanged(2 /* Akonadi::AgentBase::Status::Broken */, {});
    onlineChanged(false);
}

bool AgentBrokenInstance::start(const AgentType & /*agentInfo*/)
{
    return false;
}

void AgentBrokenInstance::quit()
{
    // no-op
}

void AgentBrokenInstance::cleanup()
{
    // no-op
}

void AgentBrokenInstance::restartWhenIdle()
{
    // no-op
}

void AgentBrokenInstance::configure(qlonglong /*windowId*/)
{
    // no-op
}
