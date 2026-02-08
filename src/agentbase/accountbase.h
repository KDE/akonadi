/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include "akonadiagentbase_export.h"

#include <Akonadi/AgentBase>

#include <memory>

namespace Akonadi
{
class AccountBasePrivate;

/*!
 * \class Akonadi::AccountBase
 * \inheaderfile Akonadi/AccountBase
 * \inmodule AkonadiAgentBase
 *
 * Base class for agents that support integration with online accounts.
 *
 * Inherit from this additionally to Akonadi::AgentBase (or Akonadi::ResourceBase).
 */
class AKONADIAGENTBASE_EXPORT AccountBase
{
public:
    /*!
     *
     */
    explicit AccountBase(AgentBase *agent);
    virtual ~AccountBase();

    /*!
     * Called when the account is initially configured
     */
    virtual void initAccount() = 0;

private:
    Q_DISABLE_COPY(AccountBase)
    std::unique_ptr<AccountBasePrivate> const d;
};
} // namespace Akonadi
