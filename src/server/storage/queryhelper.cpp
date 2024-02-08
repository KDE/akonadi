/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "queryhelper.h"

#include "storage/querybuilder.h"

#include "private/imapset_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

void QueryHelper::setToQuery(const ImapSet &set, const QString &column, QueryBuilder &qb)
{
    auto newSet = set;
    newSet.optimize();
    Query::Condition cond(Query::Or);
    const auto intervals = newSet.intervals();
    for (const ImapInterval &i : intervals) {
        if (i.hasDefinedBegin() && i.hasDefinedEnd()) {
            if (i.size() == 1) {
                cond.addValueCondition(column, Query::Equals, i.begin());
            } else {
                if (i.begin() != 1) { // 1 is our standard lower bound, so we don't have to check for it explicitly
                    Query::Condition subCond(Query::And);
                    subCond.addValueCondition(column, Query::GreaterOrEqual, i.begin());
                    subCond.addValueCondition(column, Query::LessOrEqual, i.end());
                    cond.addCondition(subCond);
                } else {
                    cond.addValueCondition(column, Query::LessOrEqual, i.end());
                }
            }
        } else if (i.hasDefinedBegin()) {
            if (i.begin() != 1) { // 1 is our standard lower bound, so we don't have to check for it explicitly
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
