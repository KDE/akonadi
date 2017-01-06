/*
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
#ifndef AGENTTHREADINSTANCE_H
#define AGENTTHREADINSTANCE_H

#include "agentinstance.h"
#include "agenttype.h"

namespace Akonadi
{

class AgentThreadInstance : public AgentInstance
{
    Q_OBJECT
public:
    explicit AgentThreadInstance(AgentManager *manager);

    bool start(const AgentType &agentInfo) Q_DECL_OVERRIDE;
    void quit() Q_DECL_OVERRIDE;
    void restartWhenIdle() Q_DECL_OVERRIDE;
    void configure(qlonglong windowId) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void agentServerRegistered();

private:
    AgentType mAgentType;
};

}

#endif // AGENTTHREADINSTANCE_H
