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
#include "protocol_p.h"
#include "storage/selectquerybuilder.h"
#include "storage/itemqueryhelper.h"
#include "search/searchtaskmanager.h"
#include "akdebug.h"

using namespace Akonadi::Server;

SearchResult::SearchResult(Scope::SelectionScope scope)
    : Handler()
    , mScope(scope)
{
}

SearchResult::~SearchResult()
{
}

bool SearchResult::parseStream()
{
    const QByteArray searchId = m_streamParser->readString();
    const qint64 collectionId = m_streamParser->readNumber();
    if (mScope.scope() != Scope::Uid && mScope.scope() != Scope::Rid) {
        fail(searchId, "Only UID or RID scopes are allowed in SEARECH_RESULT");
        return false;
    }

    if (mScope.scope() == Scope::Uid) {
        // Handle empty UID set
        m_streamParser->beginList();
        if (!m_streamParser->atListEnd()) {
            mScope.parseScope(m_streamParser);
            // FIXME: A hack to move stream parser beyond ')'
            m_streamParser->readChar();
        }
    } else {
        mScope.parseScope(m_streamParser);
    }

    QSet<qint64> ids;
    if (mScope.scope() == Scope::Rid && !mScope.ridSet().isEmpty()) {
        QueryBuilder qb(PimItem::tableName());
        qb.addColumn(PimItem::idFullColumnName());
        ItemQueryHelper::remoteIdToQuery(mScope.ridSet(), connection()->context(), qb);
        qb.addValueCondition(PimItem::collectionIdFullColumnName(), Query::Equals, collectionId);

        if (!qb.exec()) {
            fail(searchId, "Failed to convert RID to UID");
            return false;
        }

        QSqlQuery query = qb.query();
        while (query.next()) {
            ids << query.value(0).toLongLong();
        }
    } else if (mScope.scope() == Scope::Uid && !mScope.uidSet().isEmpty()) {
        Q_FOREACH (const ImapInterval &interval, mScope.uidSet().intervals()) {
            if (!interval.hasDefinedBegin() && !interval.hasDefinedEnd()) {
                fail(searchId, "Open UID intervals not allowed in SEARCH_RESULT");
                return false;
            }

            for (int i = interval.begin(); i <= interval.end(); ++i) {
                ids << i;
            }
        }
    }
    SearchTaskManager::instance()->pushResults(searchId, ids, connection());

    successResponse("Done");
    return true;
}

void SearchResult::fail(const QByteArray &searchId, const char *error)
{
    SearchTaskManager::instance()->pushResults(searchId, QSet<qint64>(), connection());
    failureResponse(error);
}
