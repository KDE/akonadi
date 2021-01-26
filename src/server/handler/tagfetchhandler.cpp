/***************************************************************************
 *   SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>       *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "tagfetchhandler.h"
#include "connection.h"
#include "tagfetchhelper.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagFetchHandler::TagFetchHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool TagFetchHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::FetchTagsCommand>(m_command);

    if (!checkScopeConstraints(cmd.scope(), Scope::Uid)) {
        return failureResponse("Only UID-based TAGFETCH is supported");
    }

    TagFetchHelper helper(connection(), cmd.scope(), cmd.fetchScope());
    if (!helper.fetchTags()) {
        return failureResponse("Failed to fetch tags");
    }

    return successResponse<Protocol::FetchTagsResponse>();
}
