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
#include "status.h"

#include <QtCore/QDebug>

#include "akonadi.h"
#include "connection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/countquerybuilder.h"
#include "storage/collectionstatistics.h"

#include "response.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"

#include <private/protocol_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Status::parseStream()
{
    Protocol::FetchCollectionStatsCommand cmd;
    mInStream >> cmd;

    const Collection col = HandlerHelper::collectionFromScope(cmd.collection());
    if (!col.isValid()) {
        return failureResponse<Protocol::FetchCollectionStatsResponse>(
            QStringLiteral("No status for this folder"));
    }

    const CollectionStatistics::Statistics &stats = CollectionStatistics::self()->statistics(col);
    if (stats.count == -1) {
        return failureResponse<Protocol::FetchCollectionStatsResponse>(
            QStringLiteral("Failed to query statistics."));
    }

    mOutStream << Protocol::FetchCollectionStatsResponse(stats.count,
                                                         stats.count - stats.read,
                                                         stats.size);
    return true;
}
