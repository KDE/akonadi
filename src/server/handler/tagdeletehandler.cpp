/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "tagdeletehandler.h"

#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/tagqueryhelper.h"

#include "private/scope_p.h"
#include "storage/transaction.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagDeleteHandler::TagDeleteHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool TagDeleteHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::DeleteTagCommand>(m_command);

    if (!checkScopeConstraints(cmd.tag(), {Scope::Uid})) {
        return failureResponse(QStringLiteral("Only UID-based TAGREMOVE is supported"));
    }

    SelectQueryBuilder<Tag> tagQuery;
    TagQueryHelper::scopeToQuery(cmd.tag(), connection()->context(), tagQuery);
    if (!tagQuery.exec()) {
        return failureResponse(QStringLiteral("Failed to obtain tags"));
    }

    const Tag::List tags = tagQuery.result();

    Transaction transaction(storageBackend(), QStringLiteral("TagDeleteHandler"));

    if (!storageBackend()->removeTags(tags)) {
        return failureResponse(QStringLiteral("Failed to remove tags"));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Failed to commit transaction"));
    }

    return successResponse<Protocol::DeleteTagResponse>();
}
