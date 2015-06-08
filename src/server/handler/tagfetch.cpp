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
#include "connection.h"
#include "tagfetchhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

bool TagFetch::parseStream()
{
    Protocol::FetchTagsCommand cmd(m_command);

    if (!checkScopeConstraints(cmd.scope(), Scope::Uid)) {
        return failureResponse("Only UID-based TAGFETCH is supported");
    }

    TagFetchHelper helper(connection(),  cmd.scope());
    if (!helper.fetchTags()) {
        return failureResponse("Failed to fetch tags");
    }

    return successResponse<Protocol::FetchTagsResponse>();
}
