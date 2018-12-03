/*
 * Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "searchresulthandler.h"

#include "connection.h"
#include "storage/querybuilder.h"
#include "storage/itemqueryhelper.h"
#include "search/searchtaskmanager.h"

#include <private/imapset_p.h>
#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool SearchResultHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::SearchResultCommand>(m_command);

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
        query.finish();
    } else if (cmd.result().scope() == Scope::Uid && !cmd.result().isEmpty()) {
        const ImapSet result = cmd.result().uidSet();
        const ImapInterval::List lstInterval = result.intervals();
        for (const ImapInterval &interval : lstInterval) {
            for (qint64 i = interval.begin(), end = interval.end(); i <= end; ++i) {
                ids.insert(i);
            }
        }
    }
    SearchTaskManager::instance()->pushResults(cmd.searchId(), ids, connection());

    return successResponse<Protocol::SearchResultResponse>();
}

bool SearchResultHandler::fail(const QByteArray &searchId, const QString &error)
{
    SearchTaskManager::instance()->pushResults(searchId, QSet<qint64>(), connection());
    return failureResponse(error);
}
