/***************************************************************************
 *   Copyright (C) 2006 by Ingo Kloecker <kloecker@kde.org>                *
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

#include "collectionstatsfetchhandler.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/collectionstatistics.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool CollectionStatsFetchHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::FetchCollectionStatsCommand>(m_command);

    const Collection col = HandlerHelper::collectionFromScope(cmd.collection(), connection());
    if (!col.isValid()) {
        return failureResponse(QStringLiteral("No status for this folder"));
    }

    const CollectionStatistics::Statistics stats = CollectionStatistics::self()->statistics(col);
    if (stats.count == -1) {
        return failureResponse(QStringLiteral("Failed to query statistics."));
    }

    Protocol::FetchCollectionStatsResponse resp;
    resp.setCount(stats.count);
    resp.setUnseen(stats.count - stats.read);
    resp.setSize(stats.size);
    return successResponse(std::move(resp));
}
