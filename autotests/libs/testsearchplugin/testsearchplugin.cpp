/*
 *    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "testsearchplugin.h"

#include <QDebug>
#include <QStringList>
#include "searchquery.h"

QSet<qint64> TestSearchPlugin::search(const QString &query, const QVector<qint64> &collections, const QStringList &mimeTypes)
{
    Q_UNUSED(collections);
    Q_UNUSED(mimeTypes);
    const QSet<qint64> result = parseQuery(query);
    qDebug() << "PLUGIN QUERY:" << query;
    qDebug() << "PLUGIN RESULT:" << result;
    return parseQuery(query);
}

QSet<qint64> TestSearchPlugin::parseQuery(const QString &queryString)
{
    QSet<qint64> resultSet;
    Akonadi::SearchQuery query = Akonadi::SearchQuery::fromJSON(queryString.toLatin1());
    foreach (const Akonadi::SearchTerm &term, query.term().subTerms()) {
        if (term.key() == QLatin1String("plugin")) {
            resultSet << term.value().toInt();
        }
    }
    return resultSet;
}
