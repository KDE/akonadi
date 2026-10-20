/*
 *   SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QDBusObjectPath>
#include <QObject>

class AgentManager;

class OnlineAccountsIntegration : public QObject
{
    Q_OBJECT

public:
    explicit OnlineAccountsIntegration(AgentManager &manager);

    Q_SLOT void slotAccountCreationFinished(const QDBusObjectPath &path, const QString &xdgActivationToken);
    Q_SLOT void slotAccountRemoved(const QDBusObjectPath &path);

private:
    AgentManager &mAgentManager;
};
