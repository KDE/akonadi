/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transactionhandler.h"
#include "connection.h"
#include "storage/datastore.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TransactionHandler::TransactionHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool TransactionHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::TransactionCommand>(m_command);

    DataStore *store = connection()->storageBackend();

    switch (cmd.mode()) {
    case Protocol::TransactionCommand::Invalid:
        return failureResponse("Invalid operation");
    case Protocol::TransactionCommand::Begin:
        if (!store->beginTransaction(QStringLiteral("CLIENT TRANSACTION"))) {
            return failureResponse("Unable to begin transaction.");
        }
        break;
    case Protocol::TransactionCommand::Rollback:
        if (!store->inTransaction()) {
            return failureResponse("There is no transaction in progress.");
        }
        if (!store->rollbackTransaction()) {
            return failureResponse("Unable to roll back transaction.");
        }
        break;
    case Protocol::TransactionCommand::Commit:
        if (!store->inTransaction()) {
            return failureResponse("There is no transaction in progress.");
        }
        if (!store->commitTransaction()) {
            return failureResponse("Unable to commit transaction.");
        }
        break;
    }

    return successResponse<Protocol::TransactionResponse>();
}
