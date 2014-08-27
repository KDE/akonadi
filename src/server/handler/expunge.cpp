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

#include "expunge.h"

#include "akonadi.h"
#include "connection.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"
#include "imapstreamparser.h"

using namespace Akonadi;
using namespace Akonadi::Server;

Expunge::Expunge()
    : Handler()
{
}

Expunge::~Expunge()
{
}

bool Expunge::parseStream()
{
    Response response;

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store);

    Flag flag = Flag::retrieveByName(QLatin1String("\\DELETED"));
    if (!flag.isValid()) {
        response.setError();
        response.setString("\\DELETED flag unknown");

        Q_EMIT responseAvailable(response);
        return true;
    }

    SelectQueryBuilder<PimItem> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PimItemFlagRelation::tableName(),
               PimItemFlagRelation::leftFullColumnName(), PimItem::idFullColumnName());
    qb.addValueCondition(PimItemFlagRelation::rightFullColumnName(), Query::Equals, flag.id());

    if (qb.exec()) {
        const QVector<PimItem> items = qb.result();
        if (store->cleanupPimItems(items)) {
            // FIXME: Change the protocol to EXPUNGE + list of removed ids
            Q_FOREACH (const PimItem &item, items) {
                response.setUntagged();
                // IMAP protocol violation: should actually be the sequence number
                response.setString(QByteArray::number(item.id()) + " EXPUNGE");

                Q_EMIT responseAvailable(response);
            }
        } else {
            response.setTag(tag());
            response.setError();
            response.setString("internal error");

            Q_EMIT responseAvailable(response);
            return true;
        }
    } else {
        throw HandlerException("Unable to execute query.");
    }

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction.");
    }

    response.setTag(tag());
    response.setSuccess();
    response.setString("EXPUNGE completed");

    Q_EMIT responseAvailable(response);
    return true;
}
