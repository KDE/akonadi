/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TRANSACTIONHANDLER_H_
#define AKONADI_TRANSACTIONHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for transaction commands (BEGIN, COMMIT, ROLLBACK).
*/
class TransactionHandler : public Handler
{
public:
    TransactionHandler(AkonadiServer &akonadi);
    ~TransactionHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
