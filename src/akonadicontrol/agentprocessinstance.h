/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AGENTPROCESSINSTANCE_H
#define AGENTPROCESSINSTANCE_H

#include "agentinstance.h"

namespace Akonadi
{

class ProcessControl;

class AgentProcessInstance : public AgentInstance
{
    Q_OBJECT

public:
    explicit AgentProcessInstance(AgentManager &manager);
    ~AgentProcessInstance() override = default;

    bool start(const AgentType &agentInfo) override;
    void quit() override;
    void cleanup() override;
    void restartWhenIdle() override;
    void configure(qlonglong windowId) override;

private Q_SLOTS:
    void failedToStart();

private:
    std::unique_ptr<Akonadi::ProcessControl> mController;
};

}

#endif // AGENTPROCESSINSTANCE_H
