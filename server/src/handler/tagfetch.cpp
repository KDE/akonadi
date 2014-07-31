/***************************************************************************
 *   Copyright (C) 2014 by Daniel Vrátil <dvratil@redhat.com>              *
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

#include "tagfetch.h"
#include "imapstreamparser.h"
#include "connection.h"
#include "libs/imapset_p.h"
#include "libs/protocol_p.h"
#include "tagfetchhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagFetch::TagFetch(Scope::SelectionScope scope)
    : Handler()
    , mScope(scope)
{
}

TagFetch::~TagFetch()
{
}

bool TagFetch::parseStream()
{
    if (mScope.scope() != Scope::Uid) {
        throw HandlerException("Only UID-based TAGFETCH is supported");
    }

    mScope.parseScope(m_streamParser);

    TagFetchHelper helper(connection(),  mScope.uidSet());
    connect(&helper, SIGNAL(responseAvailable(Akonadi::Server::Response)),
            this, SIGNAL(responseAvailable(Akonadi::Server::Response)));

    if (!helper.fetchTags(AKONADI_CMD_TAGFETCH)) {
        return false;
    }

    return successResponse("UID TAGFETCH completed");
}
