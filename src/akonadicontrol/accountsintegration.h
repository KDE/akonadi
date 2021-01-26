/***************************************************************************
 *   SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef ACCOUNTSINTEGRATION_H_
#define ACCOUNTSINTEGRATION_H_

#include <QMap>
#include <QObject>

#include <Accounts/Manager>

#include <optional>

namespace Accounts
{
class Account;
class Service;
}

class AgentManager;

class AccountsIntegration : public QObject
{
    Q_OBJECT
public:
    explicit AccountsIntegration(AgentManager &agentManager);

private Q_SLOTS:
    void onAccountAdded(Accounts::AccountId);
    void onAccountRemoved(Accounts::AccountId);
    void onAccountServiceEnabled(const QString &service, bool enabled);

private:
    void configureAgentInstance(const QString &identifier, Accounts::AccountId accountId, int attempt = 0);
    std::optional<QString> agentForAccount(const QString &agentType, Accounts::AccountId accountId) const;
    void createAgent(const QString &agentType, Accounts::AccountId accountId);
    void removeAgentInstance(const QString &identifier);

    AgentManager &mAgentManager;
    Accounts::Manager mAccountsManager;

    QMap<QString, QString /* agent type */> mSupportedServices;
};

#endif
