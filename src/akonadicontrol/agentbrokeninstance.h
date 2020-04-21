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

#ifndef AGENTBROKENINSTANCE_H
#define AGENTBROKENINSTANCE_H

#include "agentinstance.h"

namespace Akonadi
{

class ProcessControl;

class AgentBrokenInstance : public AgentInstance
{
    Q_OBJECT

public:
    explicit AgentBrokenInstance(const QString &type, AgentManager &manager);
    ~AgentBrokenInstance() override = default;

    bool start(const AgentType &agentInfo) override;
    void quit() override;
    void cleanup() override;
    void restartWhenIdle() override;
    void configure(qlonglong windowId) override;
};

}

#endif
