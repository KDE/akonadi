/*
 * Copyright 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "searchresult.h"
#include "connection.h"
#include "imapstreamparser.h"
#include "storage/selectquerybuilder.h"
#include "storage/itemqueryhelper.h"
#include "search/searchtaskmanager.h"

#include <private/protocol_p.h>
#include <shared/akdebug.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool SearchResult::parseStream()
{
    Protocol::SearchResultCommand cmd;
    mInStream >> cmd;

    if (!checkScopeConstraints(cmd.result(), Scope::Uid | Scope::Rid)) {
        return fail(cmd.searchId(), QStringLiteral("Only UID or RID scopes are allowed in SEARECH_RESULT"));
    }

    QSet<qint64> ids;
    if (cmd.result().scope() == Scope::Rid && !cmd.result().isEmpty()) {
        QueryBuilder qb(PimItem::tableName());
        qb.addColumn(PimItem::idFullColumnName());
        ItemQueryHelper::remoteIdToQuery(cmd.result().ridSet(), connection()->context(), qb);
        qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::Equals, cmd.collectionId());

        if (!qb.exec()) {
            return fail(cmd.searchId(), QStringLiteral("Failed to convert RID to UID"));
        }

        QSqlQuery query = qb.query();
        while (query.next()) {
            ids << query.value(0).toLongLong();
        }
    } else if (cmd.result().scope() == Scope::Uid && !cmd.result().isEmpty()) {
        const QVector<qint64> result = cmd.result().uidSet();
        ids.reserve(result.size());
        for (qint64 id : result) {
            ids.insert(id);
        }
    }
    SearchTaskManager::instance()->pushResults(cmd.searchId(), ids, connection());

    mOutStream << Protocol::SearchResultResponse();
    return true;
}

bool SearchResult::fail(const QByteArray &searchId, const QString &error)
{
    SearchTaskManager::instance()->pushResults(searchId, QSet<qint64>(), connection());
    return failureResponse<Protocol::SearchResultResponse>(error);
}
