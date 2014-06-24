/*
 *    Copyright (c) 2014  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 *    This library is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU Library General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at your
 *    option) any later version.
 *
 *    This library is distributed in the hope that it will be useful, but WITHOUT
 *    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *    License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to the
 *    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 */

#include "testsearchplugin.h"

#include <QDebug>

#include "searchquery.h"

QSet<qint64> TestSearchPlugin::search( const QString &query, const QList<qint64> &collections, const QStringList &mimeTypes )
{
    qDebug() << query;
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

Q_EXPORT_PLUGIN2(testsearchplugin, TestSearchPlugin)
