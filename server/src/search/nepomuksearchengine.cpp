/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "nepomuksearchengine.h"

#include <akdebug.h>
#include "src/nepomuk/result.h"

#include "entities.h"
#include "storage/notificationcollector.h"
#include "storage/selectquerybuilder.h"
#include "notificationmanager.h"
#include "libs/xdgbasedirs_p.h"
#include "libs/protocol_p.h"

#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QMetaObject>
#include <qdbusservicewatcher.h>
#include <qdbusconnection.h>
#include <QSettings>

using namespace Akonadi;
using namespace Akonadi::Server;

static qint64 resultToId(const Nepomuk::Query::Result &result)
{
    const Soprano::Node &property = result.requestProperty(QUrl(QLatin1String("http://akonadi-project.org/ontologies/aneo#akonadiItemId")));
    if (!(property.isValid() && property.isLiteral() && property.literal().isString())) {
        qWarning() << "Failed to get requested akonadiItemId property";
        qDebug() << "AkonadiItemId missing in query results!" << result.resourceUri()
                 << property.isValid() << property.isLiteral()
                 << property.literal().isString() << property.literal().type()
                 << result.requestProperties().size();
//    qDebug() << result.requestProperties().values().first().toString();
        return -1;
    }
    return property.literal().toString().toLongLong();
}

NepomukSearchEngine::NepomukSearchEngine(QObject *parent)
    : QObject(parent)
    , mCollector(new NotificationCollector(this))
    , m_liveSearchEnabled(true)
{
    NotificationManager::self()->connectNotificationCollector(mCollector);

    QDBusServiceWatcher *watcher =
        new QDBusServiceWatcher(QLatin1String("org.kde.nepomuk.services.nepomukqueryservice"),
                                QDBusConnection::sessionBus(),
                                QDBusServiceWatcher::WatchForRegistration, this);
    connect(watcher, SIGNAL(serviceRegistered(QString)), SLOT(reloadSearches()));

    if (Nepomuk::Query::QueryServiceClient::serviceAvailable()) {
        reloadSearches();
    } else {
        // FIXME: try to start the nepomuk server
        akError() << "Nepomuk Query Server not available";
    }

    const QSettings settings(XdgBaseDirs::akonadiServerConfigFile(), QSettings::IniFormat);
    m_liveSearchEnabled = settings.value(QLatin1String("Nepomuk/LiveSearchEnabled"), false).toBool();
}

NepomukSearchEngine::~NepomukSearchEngine()
{
    stopSearches();
}

void NepomukSearchEngine::addSearch(const Collection &collection)
{
    const QStringList queryAttributes = collection.queryAttributes().split(QLatin1Char(' '));
    const int index = queryAttributes.indexOf(QLatin1String(AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG));
    if (index == -1 || queryAttributes.size() < index || queryAttributes.value(index + 1) != QLatin1String("SPARQL")) {
        return;
    }

    const QString &q = collection.queryString();

    if (q.size() >= 32768) {
        qWarning() << "The query is at least 32768 chars long, which is the maximum size supported by the akonadi db schema. The query is therefore most likely truncated and will not be executed.";
        return;
    }

    //FIXME the requested property must be passed to here from the calling code
    //Ideally the Query is passed as object so we can check here for the akonadiItemId property, and add it if missing
    if (!q.contains(QString::fromLatin1("reqProp1"))
        || !q.contains(QString::fromLatin1("http://akonadi-project.org/ontologies/aneo#akonadiItemId"))) {
        qWarning() << "The query MUST contain exactly one required property (http://akonadi-project.org/ontologies/aneo#akonadiItemId), if another property is additionally requested or the akonadiItemId is missing the search will fail (due to this hack)";
        qWarning() << q;
        return;
    }

    Nepomuk::Query::QueryServiceClient *query = new Nepomuk::Query::QueryServiceClient(this);

    connect(query, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
            this, SLOT(hitsAdded(QList<Nepomuk::Query::Result>)));
    connect(query, SIGNAL(entriesRemoved(QList<Nepomuk::Query::Result>)),
            this, SLOT(hitsRemoved(QList<Nepomuk::Query::Result>)));
    connect(query, SIGNAL(finishedListing()),
            this, SLOT(finishedListing()));

    mMutex.lock();
    mQueryMap.insert(query, collection.id());
    mQueryInvMap.insert(collection.id(), query);   // needed for fast lookup in removeSearch()
    mMutex.unlock();

    QHash<QString, QString> encodedRps;
    encodedRps.insert(QString::fromLatin1("reqProp1"),
                      QUrl(QString::fromLatin1("http://akonadi-project.org/ontologies/aneo#akonadiItemId")).toString());
    //FIXME hack because the reqProp is not passed to here by the caller
    query->query(collection.queryString(), encodedRps);
}

void NepomukSearchEngine::removeSearch(qint64 collectionId)
{
    Nepomuk::Query::QueryServiceClient *query = mQueryInvMap.value(collectionId);
    if (!query) {
        qWarning() << "Nepomuk QueryServer: Query could not be removed!";
        return;
    }

    query->close();

    // cleanup mappings
    mMutex.lock();
    mQueryInvMap.remove(collectionId);
    mQueryMap.remove(query);
    mMutex.unlock();

    query->deleteLater();
}

void NepomukSearchEngine::reloadSearches()
{
    akDebug() << this << sender();
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition(Collection::queryAttributesFullColumnName(), Query::Like, QLatin1String("%SPARQL%"));
    if (!qb.exec()) {
        qWarning() << "Nepomuk QueryServer: Unable to execute query!";
        return;
    }

    Q_FOREACH (const Collection &collection, qb.result()) {
        mMutex.lock();
        if (mQueryInvMap.contains(collection.id())) {
            mMutex.unlock();
            akDebug() << "updating search" << collection.name();
            removeSearch(collection.id());
        } else {
            mMutex.unlock();
            akDebug() << "adding search" << collection.name();
        }
        addSearch(collection);
    }
}

void NepomukSearchEngine::stopSearches()
{
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition(Collection::queryAttributesFullColumnName(), Query::Like, QLatin1String("%SPARQL%"));
    if (!qb.exec()) {
        qWarning() << "Nepomuk QueryServer: Unable to execute query!";
        return;
    }

    Q_FOREACH (const Collection &collection, qb.result()) {
        akDebug() << "removing search" << collection.name();
        removeSearch(collection.id());
    }
}

void NepomukSearchEngine::hitsAdded(const QList<Nepomuk::Query::Result> &entries)
{
    Nepomuk::Query::QueryServiceClient *query = qobject_cast<Nepomuk::Query::QueryServiceClient *>(sender());
    if (!query) {
        qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
        return;
    }

    mMutex.lock();
    qint64 collectionId = mQueryMap.value(query);
    mMutex.unlock();
    const Collection collection = Collection::retrieveById(collectionId);

    PimItem::List items;
    Q_FOREACH (const Nepomuk::Query::Result &result, entries) {
        const qint64 itemId = resultToId(result);

        if (itemId == -1) {
            continue;
        }

        Entity::addToRelation<CollectionPimItemRelation>(collectionId, itemId);
        items << PimItem::retrieveById(itemId);
    }
    mCollector->itemsLinked(items, collection);
    mCollector->dispatchNotifications();
}

void NepomukSearchEngine::hitsRemoved(const QList<Nepomuk::Query::Result> &entries)
{
    Nepomuk::Query::QueryServiceClient *query = qobject_cast<Nepomuk::Query::QueryServiceClient *>(sender());
    if (!query) {
        qWarning() << "Nepomuk QueryServer: Got signal from non-existing search query!";
        return;
    }

    mMutex.lock();
    qint64 collectionId = mQueryMap.value(query);
    mMutex.unlock();
    const Collection collection = Collection::retrieveById(collectionId);
    PimItem::List items;
    Q_FOREACH (const Nepomuk::Query::Result &result, entries) {
        const qint64 itemId = resultToId(result);

        if (itemId == -1) {
            continue;
        }

        Entity::removeFromRelation<CollectionPimItemRelation>(collectionId, itemId);
        items << PimItem::retrieveById(itemId);
    }

    mCollector->itemsUnlinked(items, collection);
    mCollector->dispatchNotifications();
}

void NepomukSearchEngine::finishedListing()
{
    if (m_liveSearchEnabled) {
        return;
    }

    Nepomuk::Query::QueryServiceClient *query = qobject_cast<Nepomuk::Query::QueryServiceClient *>(sender());
    if (!query) {
        qWarning() << Q_FUNC_INFO << "Nepomuk QueryServer: Got signal from non-existing search query!";
        return;
    }

    mMutex.lock();
    qint64 collectionId = mQueryMap.value(query);
    mMutex.unlock();

    removeSearch(collectionId);
}
