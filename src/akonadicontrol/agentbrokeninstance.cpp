/*
    Copyright (c) 2020 Daniel Vrátil <dvratil@kde.org>

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

#include "agentbrokeninstance.h"

using namespace Akonadi;

AgentBrokenInstance::AgentBrokenInstance(const QString &type, AgentManager &manager)
    : AgentInstance(manager)
{
    setAgentType(type);
    statusChanged(2 /* Akonadi::AgentBase::Status::Broken */, {});
    onlineChanged(false);
}

bool AgentBrokenInstance::start(const AgentType &)
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

void AgentBrokenInstance::configure(qlonglong)
{
    // no-op
}
