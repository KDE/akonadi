/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
