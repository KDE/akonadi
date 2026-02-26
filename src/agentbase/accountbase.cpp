/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "accountbase.h"
#include "accountbase_p.h"

#include "accountadaptor.h"

using namespace Qt::Literals;

namespace Akonadi
{

AccountBase::AccountBase(AgentBase *agent)
    : d(new AccountBasePrivate(this, agent))
{
}

AccountBase::~AccountBase() = default;

AccountBasePrivate::AccountBasePrivate(AccountBase *qq, AgentBase *agent)
    : q(qq)
    , agent(agent)
{
    new Akonadi__AccountAdaptor(this);
    QDBusConnection::sessionBus().registerObject(u"/Account"_s, this, QDBusConnection::ExportAdaptors);
}

QString AccountBasePrivate::accountId() const
{
    return agent->accountId();
}

void AccountBasePrivate::setAccountId(const QString &accountId)
{
    agent->setAccountId(accountId);

    q->initAccount();

    Q_EMIT accountIdChanged(accountId);
}

} // namespace Akonadi

#include "moc_accountbase_p.cpp"
