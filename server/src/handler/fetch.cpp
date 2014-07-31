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

#include "fetch.h"

#include "akonadi.h"
#include "connection.h"
#include "fetchhelper.h"
#include "response.h"
#include "storage/selectquerybuilder.h"
#include "imapstreamparser.h"
#include "cachecleaner.h"

#include <libs/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

Fetch::Fetch(Scope::SelectionScope scope)
    : mScope(scope)
{
}

bool Fetch::parseStream()
{
    // sequence set
    mScope.parseScope(m_streamParser);

    // context
    connection()->context()->parseContext(m_streamParser);
    // We require context in case we do RID fetch
    if (connection()->context()->isEmpty() && mScope.scope() == Scope::Rid) {
        throw HandlerException("No FETCH context specified");
    }

    CacheCleanerInhibitor inhibitor;

    FetchHelper fetchHelper(connection(), mScope, FetchScope(m_streamParser));
    connect(&fetchHelper, SIGNAL(responseAvailable(Akonadi::Server::Response)),
            this, SIGNAL(responseAvailable(Akonadi::Server::Response)));

    if (!fetchHelper.fetchItems(AKONADI_CMD_ITEMFETCH)) {
        return false;
    }

    if (mScope.scope() == Scope::Uid) {
        successResponse("UID FETCH completed");
    } else if (mScope.scope() == Scope::Rid) {
        successResponse("RID FETCH completed");
    } else if (mScope.scope() == Scope::Gid) {
        successResponse("GID FETCH completed");
    } else {
        successResponse("FETCH completed");
    }

    return true;
}
