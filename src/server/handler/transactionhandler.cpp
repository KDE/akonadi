/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "transactionhandler.h"
#include "connection.h"
#include "storage/datastore.h"

using namespace Akonadi;
using namespace Akonadi::Server;

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
