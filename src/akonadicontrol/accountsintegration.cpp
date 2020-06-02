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

#include "accountsintegration.h"
#include "agentmanager.h"
#include "akonadicontrol_debug.h"
#include "accountsinterface.h"

#include <private/dbus_p.h>

#include <QDBusConnection>
#include <QTimer>

#include <Accounts/Account>
#include <Accounts/Service>

using namespace Akonadi;
using namespace std::chrono_literals;

namespace {

const auto akonadiAgentType = QStringLiteral("akonadi/agentType");

}

AccountsIntegration::AccountsIntegration(AgentManager &agentManager)
    : mAgentManager(agentManager)
{
    connect(&mAccountsManager, &Accounts::Manager::accountCreated,
            this, &AccountsIntegration::onAccountAdded);
    connect(&mAccountsManager, &Accounts::Manager::accountRemoved,
            this, &AccountsIntegration::onAccountRemoved);

    const auto accounts = mAccountsManager.accountList();
    for (const auto account : accounts) {
        connect(mAccountsManager.account(account), &Accounts::Account::enabledChanged,
                this, &AccountsIntegration::onAccountServiceEnabled);
    }
}

std::optional<QString> AccountsIntegration::agentForAccount(const QString &agentType, Accounts::AccountId accountId) const
{
    const auto instances = mAgentManager.agentInstances();
    for (const auto &identifier : instances) {
        if (mAgentManager.agentInstanceType(identifier) == agentType) {
            const auto serviceName = Akonadi::DBus::agentServiceName(identifier, Akonadi::DBus::Resource);
            org::kde::Akonadi::Accounts accountsIface(serviceName, QStringLiteral("/Accounts"), QDBusConnection::sessionBus());
            if (!accountsIface.isValid()) {
                continue;
            }

            if (accountsIface.getAccountId() == accountId) {
                return identifier;
            }
        }
    }
    return std::nullopt;
}

void AccountsIntegration::configureAgentInstance(const QString &identifier, Accounts::AccountId accountId, int attempt)
{
    const auto serviceName = Akonadi::DBus::agentServiceName(identifier, Akonadi::DBus::Resource);
    org::kde::Akonadi::Accounts accountsIface(serviceName, QStringLiteral("/Accounts"), QDBusConnection::sessionBus());
    if (!accountsIface.isValid()) {
        if (attempt >= 3) {
            qCWarning(AKONADICONTROL_LOG) << "The resource" << identifier << "does not provide the Accounts DBus interface. Will remove the agent";
            mAgentManager.removeAgentInstance(identifier);
        } else {
            QTimer::singleShot(2s, this, [this, identifier, accountId, attempt]() {
                configureAgentInstance(identifier, accountId, attempt + 1);
            });
        }
        return;
    }

    accountsIface.setAccountId(accountId);
    qCDebug(AKONADICONTROL_LOG) << "Configured resource" << identifier << "for account" << accountId;
}

void AccountsIntegration::createAgent(const QString &agentType, Accounts::AccountId accountId)
{
    const auto identifier = mAgentManager.createAgentInstance(agentType);
    qCDebug(AKONADICONTROL_LOG) << "Created resource" << identifier << "for account" << accountId;
    configureAgentInstance(identifier, accountId);
}

void AccountsIntegration::removeAgentInstance(const QString &identifier)
{
    mAgentManager.removeAgentInstance(identifier);
}

void AccountsIntegration::onAccountAdded(Accounts::AccountId accId)
{
    qCDebug(AKONADICONTROL_LOG) << "Online account ID" << accId << "added.";
    auto *account = mAccountsManager.account(accId);
    if (!account || !account->isEnabled()) {
        return;
    }

    const auto enabledServices = account->enabledServices();
    for (const auto &service : enabledServices) {
        account->selectService(service);
        const auto agentType = account->valueAsString(akonadiAgentType);
        if (agentType.isEmpty()) {
            continue; // doesn't support Akonadi :-(
        }
        const auto agent = agentForAccount(agentType, accId);
        if (!agent.has_value()) {
            createAgent(agentType, account->id());
        }
        // Always go through all services, there may be more!
    }
    account->selectService();

    connect(account, &Accounts::Account::enabledChanged,
            this, &AccountsIntegration::onAccountServiceEnabled);
}

void AccountsIntegration::onAccountRemoved(Accounts::AccountId accId)
{
    qCDebug(AKONADICONTROL_LOG) << "Online account ID" << accId << "removed.";

    const auto instances = mAgentManager.agentInstances();
    for (const auto &instance : instances) {
        const auto serviceName = DBus::agentServiceName(instance, DBus::Resource);
        org::kde::Akonadi::Accounts accountIface(serviceName, QStringLiteral("/Accounts"), QDBusConnection::sessionBus());
        if (!accountIface.isValid()) {
            continue;
        }

        if (accountIface.getAccountId() == accId) {
            removeAgentInstance(instance);
        }
    }
}

void AccountsIntegration::onAccountServiceEnabled(const QString &serviceType, bool enabled)
{
    if (serviceType.isEmpty()) {
        return;
    }

    auto *account = qobject_cast<Accounts::Account*>(sender());
    qCDebug(AKONADICONTROL_LOG) << "Online account ID" << account->id() << "service" << serviceType << "has been" << (enabled ? "enabled" : "disabled");

    const auto service = mAccountsManager.service(serviceType);
    account->selectService(service);
    const auto agentType =  account->valueAsString(akonadiAgentType);
    account->selectService();
    if (agentType.isEmpty()) {
        return; // this service does not support Akonadi (yet -;)
    }

    const auto identifier = agentForAccount(agentType, account->id());
    if (enabled && !identifier.has_value()) {
        createAgent(agentType, account->id());
    } else if (!enabled && identifier.has_value()) {
        removeAgentInstance(identifier.value());
    }
}
