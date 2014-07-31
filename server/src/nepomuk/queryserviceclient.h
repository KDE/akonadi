/*
   Copyright (c) 2008-2010 Sebastian Trueg <trueg@kde.org>

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

#ifndef _NEPOMUK_QUERY_SERVICE_CLIENT_H_
#define _NEPOMUK_QUERY_SERVICE_CLIENT_H_

#include <QtCore/QObject>
#include <QtCore/QHash>

class QUrl;

namespace Nepomuk {
namespace Query {

class Result;

/**
 * \class QueryServiceClient queryserviceclient.h Nepomuk/Query/QueryServiceClient
 *
 * \brief Convenience frontend to the %Nepomuk Query DBus Service
 *
 * The QueryServiceClient provides an easy way to access the %Nepomuk Query Service
 * without having to deal with any communication details. By default it monitors
 * queries for changes.
 *
 * Usage is simple: Create an instance of the client for each search you want to
 * track. One instance may also be reused for subsequent queries if further updates
 * of the persistent query are not necessary.
 *
 * \author Sebastian Trueg <trueg@kde.org>
 */
class QueryServiceClient : public QObject
{
    Q_OBJECT

public:
    /**
     * Create a new QueryServiceClient instance.
     */
    QueryServiceClient(QObject *parent = 0);

    /**
     * Desctructor. Closes the query.
     */
    ~QueryServiceClient();

    /**
     * \brief Check if the Nepomuk query service is running.
     * \return \p true if the Nepomuk query service is running and could
     * be contacted via DBus, \p false otherwise
     */
    static bool serviceAvailable();

public Q_SLOTS:
    /**
     * Start a query using the Nepomuk query service.
     *
     * Results will be reported via newEntries. All results
     * have been reported once finishedListing has been emitted.
     *
     * \param query the query to perform.
     *
     * \return \p true if the query service was found and the query
     * was started. \p false otherwise.
     *
     * \sa QueryParser
     */
    bool query(const QString &query, const QHash<QString, QString> &encodedRps = (QHash<QString, QString>()));

    /**
     * Start a query using the Nepomuk query service.
     *
     * Results will be reported as with query(const QString&)
     * but a local event loop will be started to block the method
     * call until all results have been listed.
     *
     * The client will be closed after the initial listing. Thus,
     * changes to results will not be reported as it is the case
     * with the non-blocking methods.
     *
     * \param query the query to perform.
     *
     * \return \p true if the query service was found and the query
     * was started. \p false otherwise.
     *
     * \sa query(const QString&), close()
     */
    bool blockingQuery(const QString &query, const QHash<QString, QString> &encodedRps = (QHash<QString, QString>()));

    /**
     * Close the client, thus stop to monitor the query
     * for changes. Without closing the client it will continue
     * signalling changes to the results.
     *
     * This will also make any blockingQuery return immediately.
     */
    void close();

    /**
     * \return \p true if all results have been listed (ie. finishedListing() has
     * been emitted), close() has been called, or no query was started.
     *
     * \since 4.6
     */
    bool isListingFinished() const;

    /**
     * The last error message which has been emitted via error() or an
     * empty string if there was no error.
     *
     * \since 4.6
     */
    QString errorMessage() const;

Q_SIGNALS:
    /**
     * Emitted for new search results. This signal is emitted both
     * for the initial listing and for changes to the search.
     */
    void newEntries(const QList<Nepomuk::Query::Result> &entries);

    /**
     * Emitted if the search results changed when monitoring a query.
     * \param entries A list of resource URIs identifying the resources
     * that dropped out of the query results.
     */
    void entriesRemoved(const QList<Nepomuk::Query::Result> &entries);

    /**
     * Emitted when the initial listing has been finished, ie. if all
     * results have been reported via newEntries. If no further updates
     * are necessary the client should be closed now.
     *
     * In case of an error this signal is not emitted.
     *
     * \sa error()
     */
    void finishedListing();

    /**
     * Emitted when an error occurs. This typically happens in case the query
     * service is not running or does not respond. No further signals will be
     * emitted after this one.
     *
     * \since 4.6
     */
    void error(const QString &errorMessage);

    /**
     * Emitted when the availability of the query service changes
     *
     * \since 4.8
     */
    void serviceAvailabilityChanged(bool running);

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void _k_finishedListing())
    Q_PRIVATE_SLOT(d, void _k_handleQueryReply(QDBusPendingCallWatcher *))
    Q_PRIVATE_SLOT(d, void _k_serviceRegistered(const QString &service))
    Q_PRIVATE_SLOT(d, void _k_serviceUnregistered(const QString &service))
};
}
}

#endif
