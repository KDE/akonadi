/*
    SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentinstance.h"
#include "agenttype.h"

namespace Akonadi
{

struct UnitFileChanges {
    QString a;
    QString b;
    QString c;
};

QDBusArgument &operator<<(QDBusArgument &arg, const UnitFileChanges &changes);
const QDBusArgument &operator>>(const QDBusArgument &arg, UnitFileChanges &changes);

class AgentSystemdInstance : public AgentInstance
{
    Q_OBJECT
public:
    explicit AgentSystemdInstance(AgentManager &manager);
    ~AgentSystemdInstance() override = default;

    void cleanup() override;

    // No-ops, systemd does this for us
    bool start(const AgentType &agentInfo) override;
    void quit() override;
    void restartWhenIdle() override;
    void configure(qlonglong windowId) override;

private:
    QString unitFile(const AgentType &agentType) const;
    AgentType mAgentType;
};

} // namespace Akonadi