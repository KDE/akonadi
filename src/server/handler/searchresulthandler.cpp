/*
 * SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "searchresulthandler.h"

#include "akonadi.h"
#include "connection.h"
#include "search/searchtaskmanager.h"
#include "storage/itemqueryhelper.h"
#include "storage/querybuilder.h"

#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

SearchResultHandler::SearchResultHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool SearchResultHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::SearchResultCommand>(m_command);

    if (!checkScopeConstraints(cmd.result(), {Scope::Uid, Scope::Rid})) {
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
        const auto uidSet = cmd.result().uidSet();
        ids = QSet<qint64>(uidSet.begin(), uidSet.end());
    }
    akonadi().agentSearchManager().pushResults(cmd.searchId(), ids, connection());

    return successResponse<Protocol::SearchResultResponse>();
}

bool SearchResultHandler::fail(const QByteArray &searchId, const QString &error)
{
    akonadi().agentSearchManager().pushResults(searchId, QSet<qint64>(), connection());
    return failureResponse(error);
}
