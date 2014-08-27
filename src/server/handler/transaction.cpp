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

#include <QMetaEnum>

using namespace Akonadi::Server;

TransactionHandler::TransactionHandler(Mode mode)
    : mMode(mode)
{
}

bool TransactionHandler::parseStream()
{
    DataStore *store = connection()->storageBackend();

    if (mMode == Begin) {
        if (!store->beginTransaction()) {
            return failureResponse("Unable to begin transaction.");
        }
    }

    if (mMode == Rollback) {
        if (!store->inTransaction()) {
            return failureResponse("There is no transaction in progress.");
        }
        if (!store->rollbackTransaction()) {
            return failureResponse("Unable to roll back transaction.");
        }
    }

    if (mMode == Commit) {
        if (!store->inTransaction()) {
            return failureResponse("There is no transaction in progress.");
        }
        if (!store->commitTransaction()) {
            return failureResponse("Unable to commit transaction.");
        }
    }

    const QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("Mode"));
    return successResponse(me.valueToKey(mMode) + QByteArray(" completed"));
}
