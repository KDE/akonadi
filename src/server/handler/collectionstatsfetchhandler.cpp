/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Ingo Kloecker <kloecker@kde.org>         *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "collectionstatsfetchhandler.h"

#include "akonadi.h"
#include "connection.h"
#include "global.h"
#include "handlerhelper.h"
#include "storage/collectionstatistics.h"
#include "storage/datastore.h"

#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

CollectionStatsFetchHandler::CollectionStatsFetchHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool CollectionStatsFetchHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::FetchCollectionStatsCommand>(m_command);

    const Collection col = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
    if (!col.isValid()) {
        return failureResponse(QStringLiteral("No status for this folder"));
    }

    const auto stats = akonadi().collectionStatistics().statistics(col);
    if (stats.count == -1) {
        return failureResponse(QStringLiteral("Failed to query statistics."));
    }

    Protocol::FetchCollectionStatsResponse resp;
    resp.setCount(stats.count);
    resp.setUnseen(stats.count - stats.read);
    resp.setSize(stats.size);
    return successResponse(std::move(resp));
}
