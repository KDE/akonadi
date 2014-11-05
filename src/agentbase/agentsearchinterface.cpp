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

#include "agentsearchinterface.h"
#include "agentsearchinterface_p.h"
#include "collection.h"
#include "KDBusConnectionPool"
#include "searchresultjob_p.h"
#include "searchadaptor.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "servermanager.h"
#include "agentbase.h"
#include <akonadi/private/imapset_p.h>

using namespace Akonadi;

AgentSearchInterfacePrivate::AgentSearchInterfacePrivate(AgentSearchInterface *qq)
    : q(qq)
{
    new Akonadi__SearchAdaptor(this);
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Search"),
                                                          this, QDBusConnection::ExportAdaptors);

    QTimer::singleShot(0, this, SLOT(delayedInit()));
}

void AgentSearchInterfacePrivate::delayedInit()
{
    QDBusInterface iface(ServerManager::serviceName(ServerManager::Server),
                         QStringLiteral("/SearchManager"),
                         QStringLiteral("org.freedesktop.Akonadi.SearchManager"),
                         QDBusConnection::sessionBus(), this);
    QDBusMessage msg = iface.call(QStringLiteral("registerInstance"), dynamic_cast<AgentBase *>(q)->identifier());
}

void AgentSearchInterfacePrivate::addSearch(const QString &query, const QString &queryLanguage, quint64 resultCollectionId)
{
    q->addSearch(query, queryLanguage, Collection(resultCollectionId));
}

void AgentSearchInterfacePrivate::removeSearch(quint64 resultCollectionId)
{
    q->removeSearch(Collection(resultCollectionId));
}

void AgentSearchInterfacePrivate::search(const QByteArray &searchId,
                                         const QString &query,
                                         quint64 collectionId)
{
    mSearchId = searchId;
    mCollectionId = collectionId;

    CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection(mCollectionId), CollectionFetchJob::Base, this);
    fetchJob->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
    fetchJob->setProperty("query", query);
    connect(fetchJob, SIGNAL(finished(KJob*)), this, SLOT(collectionReceived(KJob*)));
}

void AgentSearchInterfacePrivate::collectionReceived(KJob *job)
{
    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob *>(job);
    if (fetchJob->error()) {
        qCritical() << fetchJob->errorString();
        new SearchResultJob(fetchJob->property("searchId").toByteArray(), Collection(mCollectionId), this);
        return;
    }

    if (fetchJob->collections().count() != 1) {
        qDebug() << "Server requested search in invalid collection, or collection was removed in the meanwhile";
        // Tell server we are done
        new SearchResultJob(fetchJob->property("searchId").toByteArray(), Collection(mCollectionId), this);
        return;
    }

    const Collection collection = fetchJob->collections().first();
    q->search(fetchJob->property("query").toString(),
              collection);
}

AgentSearchInterface::AgentSearchInterface()
    : d(new AgentSearchInterfacePrivate(this))
{
}

AgentSearchInterface::~AgentSearchInterface()
{
    delete d;
}

void AgentSearchInterface::searchFinished(const QVector<qint64> result, ResultScope scope)
{
    if (scope == Akonadi::AgentSearchInterface::Rid) {
        QVector<QByteArray> rids;
        rids.reserve(result.size());
        Q_FOREACH (qint64 rid, result) {
            rids << QByteArray::number(rid);
        }

        searchFinished(rids);
        return;
    }

    SearchResultJob *resultJob = new SearchResultJob(d->mSearchId, Collection(d->mCollectionId), d);
    resultJob->setResult(result);
}

void AgentSearchInterface::searchFinished(const ImapSet &result, ResultScope scope)
{
    if (scope == Akonadi::AgentSearchInterface::Rid) {
        QVector<QByteArray> rids;
        Q_FOREACH (const ImapInterval &interval, result.intervals()) {
            for (int i = interval.begin(); i <= interval.end(); ++i) {
                rids << QByteArray::number(i);
            }
        }

        searchFinished(rids);
        return;
    }

    SearchResultJob *resultJob = new SearchResultJob(d->mSearchId, Collection(d->mCollectionId), d);
    resultJob->setResult(result);
}

void AgentSearchInterface::searchFinished(const QVector<QByteArray> &result)
{
    SearchResultJob *resultJob = new SearchResultJob(d->mSearchId, Collection(d->mCollectionId), d);
    resultJob->setResult(result);
}

#include "moc_agentsearchinterface_p.cpp"
