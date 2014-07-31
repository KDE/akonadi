/*
   Copyright (c) 2008-2009 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "queryserviceclient.h"
#include <akdebug.h>
#include "dbusoperators.h"
#include "result.h"
#include "queryserviceinterface.h"
#include "queryinterface.h"
#include "dbusconnectionpool.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusReply>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

class Nepomuk::Query::QueryServiceClient::Private
{
public:
    Private()
        : queryServiceInterface(0)
        , queryInterface(0)
        , dbusConnection(Akonadi::Server::DBusConnectionPool::threadConnection())
        , m_queryActive(false)
        , loop(0)
    {
    }

    void _k_finishedListing();
    void _k_handleQueryReply(QDBusPendingCallWatcher *watcher);
    void _k_serviceRegistered(const QString &service);
    void _k_serviceUnregistered(const QString &service);

    org::kde::nepomuk::QueryService *queryServiceInterface;
    org::kde::nepomuk::Query *queryInterface;
    QDBusServiceWatcher *queryServiceWatcher;

    QueryServiceClient *q;

    QPointer<QDBusPendingCallWatcher> m_pendingCallWatcher;

    QDBusConnection dbusConnection;

    bool m_queryActive;
    QEventLoop *loop;
    QString m_errorMessage;
};

void Nepomuk::Query::QueryServiceClient::Private::_k_finishedListing()
{
    m_queryActive = false;
    Q_EMIT q->finishedListing();
    if (loop) {
        q->close();
    }
}

void Nepomuk::Query::QueryServiceClient::Private::_k_handleQueryReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        akDebug() << reply.error();
        m_errorMessage = reply.error().message();
        m_queryActive = false;
        Q_EMIT q->error(m_errorMessage);
        if (loop) {
            loop->exit();
        }
    } else {
        queryInterface = new org::kde::nepomuk::Query(queryServiceInterface->service(),
                                                      reply.value().path(),
                                                      dbusConnection);
        connect(queryInterface, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
                q, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)));
        connect(queryInterface, SIGNAL(entriesRemoved(QList<Nepomuk::Query::Result>)),
                q, SIGNAL(entriesRemoved(QList<Nepomuk::Query::Result>)));
        connect(queryInterface, SIGNAL(finishedListing()),
                q, SLOT(_k_finishedListing()));
        // run the listing async in case the event loop below is the only one we have
        // and we need it to handle the signals and list returns results immediately
        QTimer::singleShot(0, queryInterface, SLOT(list()));
    }

    delete watcher;
}

void Nepomuk::Query::QueryServiceClient::Private::_k_serviceRegistered(const QString &service)
{
    if (service == QLatin1String("org.kde.nepomuk.services.nepomukqueryservice")) {
        akDebug() << "NEP: nepomukqueryservice has registered again";
        delete queryServiceInterface;

        queryServiceInterface = new org::kde::nepomuk::QueryService(QLatin1String("org.kde.nepomuk.services.nepomukqueryservice"),
                                                                    QLatin1String("/nepomukqueryservice"),
                                                                    dbusConnection);
        Q_EMIT q->serviceAvailabilityChanged(true);
    }
}

void Nepomuk::Query::QueryServiceClient::Private::_k_serviceUnregistered(const QString &service)
{
    if (service == QLatin1String("org.kde.nepomuk.services.nepomukqueryservice")) {
        q->close();
        Q_EMIT q->serviceAvailabilityChanged(false);
    }
}

Nepomuk::Query::QueryServiceClient::QueryServiceClient(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
    d->q = this;

    Nepomuk::Query::registerDBusTypes();

    // we use our own connection to be thread-safe
    d->queryServiceInterface = new org::kde::nepomuk::QueryService(QLatin1String("org.kde.nepomuk.services.nepomukqueryservice"),
                                                                   QLatin1String("/nepomukqueryservice"),
                                                                   d->dbusConnection);
    d->queryServiceWatcher = new QDBusServiceWatcher(QLatin1String("org.kde.nepomuk.services.nepomukqueryservice"),
                                                     QDBusConnection::sessionBus(),
                                                     QDBusServiceWatcher::WatchForOwnerChange,
                                                     this);
    connect(d->queryServiceWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(_k_serviceRegistered(QString)));
    connect(d->queryServiceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(_k_serviceUnregistered(QString)));
}

Nepomuk::Query::QueryServiceClient::~QueryServiceClient()
{
    close();
    delete d->queryServiceInterface;
    delete d;
}

bool Nepomuk::Query::QueryServiceClient::query(const QString &query, const QHash<QString, QString> &encodedRps)
{
    close();

    if (d->queryServiceInterface->isValid()) {
        d->m_queryActive = true;
        d->m_pendingCallWatcher = new QDBusPendingCallWatcher(d->queryServiceInterface->asyncCall(QLatin1String("sparqlQuery"),
                                                              query,
                                                              QVariant::fromValue(encodedRps)),
                                                              this);
        connect(d->m_pendingCallWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(_k_handleQueryReply(QDBusPendingCallWatcher*)));
        return true;
    } else {
        akDebug() << "Could not contact nepomuk query service.";
        return false;
    }
}

bool Nepomuk::Query::QueryServiceClient::blockingQuery(const QString &q, const QHash<QString, QString> &encodedRps)
{
    if (query(q, encodedRps)) {
        QEventLoop loop;
        d->loop = &loop;
        loop.exec();
        d->loop = 0;
        close();
        return true;
    } else {
        return false;
    }
}

void Nepomuk::Query::QueryServiceClient::close()
{
    // drop pending query calls
    // TODO: This could lead to dangling queries in the service when close is called before the pending call has returned!!!
    //       We could also use a stack of pending calls or something like that.
    delete d->m_pendingCallWatcher;

    d->m_errorMessage.truncate(0);

    if (d->queryInterface) {
        akDebug() << Q_FUNC_INFO;
        d->queryInterface->close();
        delete d->queryInterface;
        d->queryInterface = 0;
        d->m_queryActive = false;
        if (d->loop) {
            d->loop->exit();
        }
    }
}

bool Nepomuk::Query::QueryServiceClient::isListingFinished() const
{
    return !d->m_queryActive;
}

bool Nepomuk::Query::QueryServiceClient::serviceAvailable()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String("org.kde.nepomuk.services.nepomukqueryservice"));
}

QString Nepomuk::Query::QueryServiceClient::errorMessage() const
{
    return d->m_errorMessage;
}

#include "moc_queryserviceclient.cpp"
