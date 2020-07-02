/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "itemfetchhandler.h"

#include "connection.h"
#include "itemfetchhelper.h"
#include "cachecleaner.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ItemFetchHandler::ItemFetchHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool ItemFetchHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::FetchItemsCommand>(m_command);

    CommandContext context = connection()->context();
    if (!context.setScopeContext(cmd.scopeContext())) {
        return failureResponse(QStringLiteral("Invalid scope context"));
    }

    // We require context in case we do RID fetch
    if (context.isEmpty() && cmd.scope().scope() == Scope::Rid) {
        return failureResponse(QStringLiteral("No FETCH context specified"));
    }

    CacheCleanerInhibitor inhibitor(akonadi());

    ItemFetchHelper fetchHelper(connection(), context, cmd.scope(), cmd.itemFetchScope(), cmd.tagFetchScope(), akonadi());
    if (!fetchHelper.fetchItems()) {
        return failureResponse(QStringLiteral("Failed to fetch items"));
    }

    return successResponse<Protocol::FetchItemsResponse>();
}
