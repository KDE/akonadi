/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentinstance.h"

namespace Akonadi
{
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

