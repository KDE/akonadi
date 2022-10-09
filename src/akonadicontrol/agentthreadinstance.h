/*
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "agentinstance.h"
#include "agenttype.h"

#include <QDBusServiceWatcher>

namespace Akonadi
{
class AgentThreadInstance : public AgentInstance
{
    Q_OBJECT
public:
    explicit AgentThreadInstance(AgentManager &manager);
    ~AgentThreadInstance() override = default;

    bool start(const AgentType &agentInfo) override;
    void quit() override;
    void restartWhenIdle() override;
    void configure(qlonglong windowId) override;

private Q_SLOTS:
    void agentServerRegistered();

private:
    AgentType mAgentType;
    QDBusServiceWatcher mServiceWatcher;
};

}
