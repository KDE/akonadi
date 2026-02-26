/*
 *  SPDX-FileCopyrightText: 2023 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "accountbase.h"

#include <QObject>

class Akonadi__AccountAdaptor;

namespace Akonadi
{
class AccountBasePrivate : public QObject
{
    Q_OBJECT

public:
    explicit AccountBasePrivate(AccountBase *qq, AgentBase *agent);

Q_SIGNALS:
    void accountIdChanged(const QString &accountId);

private:
    friend class AccountBase;
    friend class ::Akonadi__AccountAdaptor;

    // D-Bus calls
    QString accountId() const;
    void setAccountId(const QString &accountId);

    AccountBase *const q;
    AgentBase *agent;
};
}
