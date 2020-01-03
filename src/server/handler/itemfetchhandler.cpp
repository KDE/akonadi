/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
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
