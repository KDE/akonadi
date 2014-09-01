/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "nepomuksearch.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include "nepomuk/result.h"

using namespace Akonadi::Server;

static qint64 resultToId(const Nepomuk::Query::Result &result)
{
    const Soprano::Node &property = result.requestProperty(QUrl(QLatin1String("http://akonadi-project.org/ontologies/aneo#akonadiItemId")));
    if (!(property.isValid() && property.isLiteral() && property.literal().isString())) {
        qWarning() << "Failed to get requested akonadiItemId property";
        qDebug() << "AkonadiItemId missing in query results!" << result.resourceUri() << property.isValid() << property.isLiteral() << property.literal().isString() << property.literal().type() << result.requestProperties().size();
//    qDebug() << result.requestProperties().values().first().toString();
        return -1;
    }
    return property.literal().toString().toLongLong();
}

NepomukSearch::NepomukSearch(QObject *parent)
    : QObject(parent)
    , mSearchService(0)
{
    mSearchService = new Nepomuk::Query::QueryServiceClient(this);
    connect(mSearchService, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
            this, SLOT(hitsAdded(QList<Nepomuk::Query::Result>)));
}

NepomukSearch::~NepomukSearch()
{
    mSearchService->close();
    delete mSearchService;
}

QStringList NepomukSearch::search(const QString &query)
{
    //qDebug() << Q_FUNC_INFO << query;
    if (!mSearchService->serviceAvailable()) {
        qWarning() << "Nepomuk search service not available!";
        return QStringList();
    }

    // Insert a property request for the item Id. This should
    // be part of the query already and extracted from that,
    // but that doesn't seem to work at the moment, so be explicit.
    QHash<QString, QString> encodedRps;
    encodedRps.insert(QString::fromLatin1("reqProp1"), QUrl(QString::fromLatin1("http://akonadi-project.org/ontologies/aneo#akonadiItemId")).toString());

    if (!mSearchService->blockingQuery(query, encodedRps)) {
        qWarning() << Q_FUNC_INFO << "Calling blockingQuery() failed!" << query;
        return QStringList();
    }

    // retrieve IDs for query results without ID:
    // retrieving must happen after previous blocking query is finished to avoid that in case
    // of multiple retrieved IDs after one result the wrong blocking thread is invoked
    if (!mResultsWithoutIdProperty.isEmpty()) {
        Nepomuk::Query::QueryServiceClient idSearchService(this);
        connect(&idSearchService, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
                this, SLOT(idHitsAdded(QList<Nepomuk::Query::Result>)));

        // go fetch the akonadi item id property for those results that don't have them
        Q_FOREACH (const Nepomuk::Query::Result &result, mResultsWithoutIdProperty) {
            qDebug() << "NepomukSearch::hitsAdded() search for IDs for results without ids: " << result.resourceUri().toString();
            QHash<QString, QString> encodedRps;
            // We do another query to get the akonadiItemId attribute (since it may not have been added to the original query)
            encodedRps.insert(QString::fromLatin1("reqProp1"), QUrl(QString::fromLatin1("http://akonadi-project.org/ontologies/aneo#akonadiItemId")).toString());

            idSearchService.blockingQuery(QString::fromLatin1("SELECT DISTINCT ?r ?reqProp1 WHERE { <%1> a ?v2 . <%1> <http://akonadi-project.org/ontologies/aneo#akonadiItemId> ?reqProp1 . } LIMIT 1").arg(result.resourceUri().toString()), encodedRps);
        }
        mResultsWithoutIdProperty.clear();
    }

    return mMatchingUIDs.toList();
}

void NepomukSearch::addHit(const Nepomuk::Query::Result &result)
{
    const qint64 itemId = resultToId(result);

    if (itemId == -1) {
        return;
    }

    mMatchingUIDs.insert(QString::number(itemId));
}

void NepomukSearch::hitsAdded(const QList<Nepomuk::Query::Result> &entries)
{
    if (!mSearchService->serviceAvailable()) {
        qWarning() << "Nepomuk search service not available!";
        return;
    }

    Q_FOREACH (const Nepomuk::Query::Result &result, entries) {
        const Soprano::Node &property = result.requestProperty(QUrl(QLatin1String("http://akonadi-project.org/ontologies/aneo#akonadiItemId")));
        if (!(property.isValid() && property.isLiteral() && property.literal().isString())) {
            mResultsWithoutIdProperty << result;
        } else {
            // for those results that already have a proper akonadi item id property,
            // we can add matches right away
            addHit(result);
        }
    }
}

void NepomukSearch::idHitsAdded(const QList< Nepomuk::Query::Result > &entries)
{
    Q_FOREACH (const Nepomuk::Query::Result &result, entries) {
        addHit(result);
    }
}
