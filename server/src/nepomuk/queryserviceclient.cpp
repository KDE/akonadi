/*
   Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "queryserviceclient.h"
#include "dbusoperators.h"
#include "result.h"
#include "queryserviceinterface.h"
#include "queryinterface.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusReply>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

namespace {

    QAtomicInt s_connectionCounter;
    /**
     * Each thread needs its own QDBusConnection. We do it the very easy
     * way and just create a new connection for each client
     * Why does Qt not handle this automatically?
     */
    class QDBusConnectionPerThreadHelper
    {
    public:
        QDBusConnectionPerThreadHelper()
            : m_connection( QDBusConnection::connectToBus(
                                QDBusConnection::SessionBus,
                                QString::fromLatin1("NepomukQueryServiceConnection%1").arg(newNumber()) ) )
        {
        }
        ~QDBusConnectionPerThreadHelper() {
            QDBusConnection::disconnectFromBus( m_connection.name() );
        }

        static QDBusConnection threadConnection();

    private:
        static int newNumber() {
            return s_connectionCounter.fetchAndAddAcquire(1);
        }
        QDBusConnection m_connection;
    };

    QThreadStorage<QDBusConnectionPerThreadHelper *> s_perThreadConnection;

    QDBusConnection QDBusConnectionPerThreadHelper::threadConnection()
    {
        if (!s_perThreadConnection.hasLocalData()) {
            s_perThreadConnection.setLocalData(new QDBusConnectionPerThreadHelper);
        }
        return s_perThreadConnection.localData()->m_connection;
    }
}


class Nepomuk::Query::QueryServiceClient::Private
{
public:
    Private()
        : queryServiceInterface( 0 ),
          queryInterface( 0 ),
          dbusConnection( QDBusConnectionPerThreadHelper::threadConnection() ),
          loop( 0 ) {
    }

    void _k_entriesRemoved( const QStringList& );
    void _k_finishedListing();
    bool handleQueryReply( QDBusReply<QDBusObjectPath> reply );

    org::kde::nepomuk::QueryService* queryServiceInterface;
    org::kde::nepomuk::Query* queryInterface;

    QueryServiceClient* q;

    QDBusConnection dbusConnection;

    QEventLoop* loop;
};


void Nepomuk::Query::QueryServiceClient::Private::_k_entriesRemoved( const QStringList& uris )
{
    QList<QUrl> ul;
    foreach( const QString& s, uris ) {
        ul.append( QUrl( s ) );
    }
    emit q->entriesRemoved( ul );
}


void Nepomuk::Query::QueryServiceClient::Private::_k_finishedListing()
{
    emit q->finishedListing();
    if( loop ) {
        q->close();
    }
}

bool Nepomuk::Query::QueryServiceClient::Private::handleQueryReply( QDBusReply<QDBusObjectPath> r )
{
    if ( r.isValid() ) {
        queryInterface = new org::kde::nepomuk::Query( queryServiceInterface->service(),
                                                       r.value().path(),
                                                       dbusConnection  );
        connect( queryInterface, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)),
                 q, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)) );
        connect( queryInterface, SIGNAL(entriesRemoved(QStringList)),
                 q, SLOT(_k_entriesRemoved(QStringList)) );
        connect( queryInterface, SIGNAL(finishedListing()),
                 q, SLOT(_k_finishedListing()) );
        // run the listing async in case the event loop below is the only one we have
        // and we need it to handle the signals and list returns results immediately
        QTimer::singleShot( 0, queryInterface, SLOT(list()) );
        return true;
    }
    else {
        qDebug() << "Query failed:" << r.error().message();
        return false;
    }
}


Nepomuk::Query::QueryServiceClient::QueryServiceClient( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->q = this;

    Nepomuk::Query::registerDBusTypes();

    // we use our own connection to be thread-safe
    d->queryServiceInterface = new org::kde::nepomuk::QueryService( QLatin1String("org.kde.nepomuk.services.nepomukqueryservice"),
                                                                    QLatin1String("/nepomukqueryservice"),
                                                                    d->dbusConnection );
}


Nepomuk::Query::QueryServiceClient::~QueryServiceClient()
{
    delete d->queryServiceInterface;
    close();
    delete d;
}


bool Nepomuk::Query::QueryServiceClient::query( const QString& query )
{
    close();

    if ( d->queryServiceInterface->isValid() ) {
        return d->handleQueryReply( d->queryServiceInterface->sparqlQuery( query, QHash<QString, QString>() ) );
    }
    else {
        qDebug() << "Could not contact query service.";
        return false;
    }
}


bool Nepomuk::Query::QueryServiceClient::blockingQuery( const QString& q )
{
    if( query( q ) ) {
        QEventLoop loop;
        d->loop = &loop;
        loop.exec();
        d->loop = 0;
        return true;
    }
    else {
        return false;
    }
}


void Nepomuk::Query::QueryServiceClient::close()
{
    if ( d->queryInterface ) {
        qDebug() << Q_FUNC_INFO;
        d->queryInterface->close();
        delete d->queryInterface;
        d->queryInterface = 0;
        if( d->loop )
            d->loop->exit();
    }
}


bool Nepomuk::Query::QueryServiceClient::serviceAvailable()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered( QLatin1String("org.kde.nepomuk.services.nepomukqueryservice") );
}

#include "queryserviceclient.moc"
