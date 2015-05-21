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

#include "transaction.h"
#include "storage/datastore.h"
#include "connection.h"
#include "response.h"
#include "imapstreamparser.h"

#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool TransactionHandler::parseStream()
{
    Protocol::TransactionCommand cmd;
    mInStream >> cmd;

    DataStore *store = connection()->storageBackend();

    switch (cmd.mode()) {
    case Protocol::TransactionCommand::Begin:
        if (!store->beginTransaction()) {
            return failureResponse<Protocol::TransactionResponse>(
                QStringLiteral("Unable to begin transaction."));
        }
        break;
    case Protocol::TransactionCommand::Rollback:
        if (!store->inTransaction()) {
            return failureResponse<Protocol::TransactionResponse>(
                QStringLiteral("There is no transaction in progress."));
        }
        if (!store->rollbackTransaction()) {
            return failureResponse<Protocol::TransactionResponse>(
                QStringLiteral("Unable to roll back transaction."));
        }
        break;
    case Protocol::TransactionCommand::Commit:
        if (!store->inTransaction()) {
            return failureResponse<Protocol::TransactionResponse>(
                QStringLiteral("There is no transaction in progress."));
        }
        if (!store->commitTransaction()) {
            return failureResponse<Protocol::TransactionResponse>(
                QStringLiteral("Unable to commit transaction."));
        }
    }

    mOutStream << Protocol::TransactionResponse();
    return true;
}
