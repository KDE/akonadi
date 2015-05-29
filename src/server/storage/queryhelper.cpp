/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "queryhelper.h"

#include "storage/querybuilder.h"

#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

void QueryHelper::setToQuery(const ImapSet &set, const QString &column, QueryBuilder &qb)
{
    Query::Condition cond(Query::Or);
    Q_FOREACH (const ImapInterval &i, set.intervals()) {
        if (i.hasDefinedBegin() && i.hasDefinedEnd()) {
            if (i.size() == 1) {
                cond.addValueCondition(column, Query::Equals, i.begin());
            } else {
                if (i.begin() != 1) {   // 1 is our standard lower bound, so we don't have to check for it explicitly
                    Query::Condition subCond(Query::And);
                    subCond.addValueCondition(column, Query::GreaterOrEqual, i.begin());
                    subCond.addValueCondition(column, Query::LessOrEqual, i.end());
                    cond.addCondition(subCond);
                } else {
                    cond.addValueCondition(column, Query::LessOrEqual, i.end());
                }
            }
        } else if (i.hasDefinedBegin()) {
            if (i.begin() != 1) {   // 1 is our standard lower bound, so we don't have to check for it explicitly
                cond.addValueCondition(column, Query::GreaterOrEqual, i.begin());
            }
        } else if (i.hasDefinedEnd()) {
            cond.addValueCondition(column, Query::LessOrEqual, i.end());
        }
    }
    if (!cond.isEmpty()) {
        qb.addCondition(cond);
    }
}
