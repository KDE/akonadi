/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagdeletehandler.h"

#include "storage/datastore.h"
#include "storage/queryhelper.h"
#include "storage/selectquerybuilder.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

TagDeleteHandler::TagDeleteHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool TagDeleteHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::DeleteTagCommand>(m_command);

    if (!checkScopeConstraints(cmd.tag(), Scope::Uid)) {
        return failureResponse(QStringLiteral("Only UID-based TAGREMOVE is supported"));
    }

    SelectQueryBuilder<Tag> tagQuery;
    QueryHelper::setToQuery(cmd.tag().uidSet(), Tag::idFullColumnName(), tagQuery);
    if (!tagQuery.exec()) {
        return failureResponse(QStringLiteral("Failed to obtain tags"));
    }

    const Tag::List tags = tagQuery.result();

    if (!storageBackend()->removeTags(tags)) {
        return failureResponse(QStringLiteral("Failed to remove tags"));
    }

    return successResponse<Protocol::DeleteTagResponse>();
}
