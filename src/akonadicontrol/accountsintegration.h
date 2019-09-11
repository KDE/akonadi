/***************************************************************************
 *   Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef ACCOUNTSINTEGRATION_H_
#define ACCOUNTSINTEGRATION_H_

#include <QObject>
#include <QMap>

#include <Accounts/Manager>

#include <shared/akoptional.h>

namespace Accounts
{
class Account;
class Service;
}

class AgentManager;

class AccountsIntegration: public QObject
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
    Akonadi::akOptional<QString> agentForAccount(const QString &agentType, Accounts::AccountId accountId) const;
    void createAgent(const QString &agentType, Accounts::AccountId accountId);
    void removeAgentInstance(const QString &identifier);
    void loadSupportedServices();

    AgentManager &mAgentManager;
    Accounts::Manager mAccountsManager;


    QMap<QString, QString /* agent type */> mSupportedServices;
};



#endif
