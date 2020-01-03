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

#include "searchcreatehandler.h"


#include "akonadi.h"
#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "search/searchmanager.h"
#include <shared/akranges.h>
#include "akonadiserver_debug.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

SearchCreateHandler::SearchCreateHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

bool SearchCreateHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::StoreSearchCommand>(m_command);

    if (cmd.name().isEmpty()) {
        return failureResponse("No name specified");
    }

    if (cmd.query().isEmpty()) {
        return failureResponse("No query specified");
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db, QStringLiteral("SEARCH PERSISTENT"));

    QStringList queryAttributes;

    if (cmd.remote()) {
        queryAttributes << QStringLiteral(AKONADI_PARAM_REMOTE);
    }
    if (cmd.recursive()) {
        queryAttributes << QStringLiteral(AKONADI_PARAM_RECURSIVE);
    }

    QVector<qint64> queryColIds = cmd.queryCollections();
    std::sort(queryColIds.begin(), queryColIds.end());
    const auto queryCollections = queryColIds | Views::transform([](const auto id) { return QString::number(id); }) | Actions::toQList;

    Collection col;
    col.setQueryString(cmd.query());
    col.setQueryAttributes(queryAttributes.join(QLatin1Char(' ')));
    col.setQueryCollections(queryCollections.join(QLatin1Char(' ')));
    col.setParentId(1);   // search root
    col.setResourceId(1);   // search resource
    col.setName(cmd.name());
    col.setIsVirtual(true);

    const QStringList lstMimeTypes = cmd.mimeTypes();
    if (!db->appendCollection(col, lstMimeTypes, {{"AccessRights", "luD"}})) {
        return failureResponse("Unable to create persistent search");
    }

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction");
    }

    akonadi().searchManager().updateSearch(col);

    sendResponse(HandlerHelper::fetchCollectionsResponse(akonadi(), col));
    return successResponse<Protocol::StoreSearchResponse>();
}
