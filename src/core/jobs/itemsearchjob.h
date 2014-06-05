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

#ifndef AKONADI_ITEMSEARCHJOB_H
#define AKONADI_ITEMSEARCHJOB_H

#include "akonadicore_export.h"
#include "item.h"
#include "job.h"
#include "collection.h"

#include <QtCore/QUrl>

namespace Akonadi {

class ItemFetchScope;
class ItemSearchJobPrivate;
class SearchQuery;

/**
 * @short Job that searches for items in the Akonadi storage.
 *
 * This job searches for items that match a given search query and returns
 * the list of matching item.
 *
 * @code
 *
 * SearchQuery query;
 * query.addTerm( SearchTerm( "From", "user1@domain.example", SearchTerm::CondEqual ) );
 * query.addTerm( SearchTerm( "Date", QDateTime( QDate( 2014, 01, 27 ), QTime( 00, 00, 00 ) ), SearchTerm::CondGreaterThan );
 *
 * Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( query );
 * job->fetchScope().fetchFullPayload();
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(searchResult(KJob*)) );
 *
 * ...
 *
 * MyClass::searchResult( KJob *job )
 * {
 *   Akonadi::ItemSearchJob *searchJob = qobject_cast<Akonadi::ItemSearchJob*>( job );
 *   const Akonadi::Item::List items = searchJob->items();
 *   foreach ( const Akonadi::Item &item, items ) {
 *     // extract the payload and do further stuff
 *   }
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADICORE_EXPORT ItemSearchJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates an item search job.
     *
     * @param query The search query in raw Akonadi search metalanguage format (JSON)
     * @param parent The parent object.
     * @deprecated Deprecated as of 4.13. Use SearchQuery instead.
     */
    explicit AKONADICORE_DEPRECATED ItemSearchJob(const QString &query, QObject *parent = 0);

    /**
     * Creates an item search job.
     *
     * @param query The search query.
     * @param parent The parent object.
     * @since 4.13
     */
    explicit ItemSearchJob(const SearchQuery &query, QObject *parent = 0);

    /**
     * Destroys the item search job.
     */
    ~ItemSearchJob();

    /**
     * Sets the search @p query.
     *
     * @since 4.13
     */
    void setQuery(const SearchQuery &query);

    /**
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an matching item's data is fetched
     * from the server, e.g. whether to fetch the full item payload or
     * only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     */
    void setFetchScope(const ItemFetchScope &fetchScope);

    /**
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * @return a reference to the current item fetch scope
     *
     * @see setFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &fetchScope();

    /**
     * Returns the items that matched the search query.
     */
    Item::List items() const;

    /**
     * Returns an URI that represents a predicate that is always added to the Nepomuk resource
     * by the Akonadi Nepomuk feeders.
     *
     * The statement containing this predicate has the Akonadi Item ID of the resource as string
     * as the object, and the Nepomuk resource, e.g. a PersonContact, as the subject.
     *
     * Always limit your searches to statements that contain this URI as predicate.
     *
     * @since 4.4.3
     * @deprecated Deprecated as of 4.13, where SPARQL queries were replaced by Baloo
     */
    static AKONADICORE_DEPRECATED QUrl akonadiItemIdUri();

    /**
     * Search only for items of given mime types.
     *
     * @since 4.13
     */
    void setMimeTypes(const QStringList &mimeTypes);

    /**
     * Returns list of mime types to search in
     *
     * @since 4.13
     */
    QStringList mimeTypes() const;

    /**
     * Search only in given collections.
     *
     * When recursive search is enabled, all child collections of each specified
     * collection will be searched too
     *
     * By default all collections are be searched.
     *
     * @param collections Collections to search
     * @since 4.13
     */
    void setSearchCollections(const Collection::List &collections);

    /**
     * Returns list of collections to search.
     *
     * This list does not include child collections that will be searched when
     * recursive search is enabled
     *
     * @since 4.13
     */
    Collection::List searchCollections() const;

    /**
     * Sets whether the search should recurse into collections
     *
     * When set to true, all child collections of the specific collections will
     * be search recursively.
     *
     * @param recursive Whether to search recursively
     * @since 4.13
     */
    void setRecursive(bool recursive);

    /**
     * Returns whether the search is recursive
     *
     * @since 4.13
     */
    bool isRecursive() const;

    /**
     * Sets whether resources should be queried too.
     *
     * When set to true, Akonadi will search local indexed items and will also
     * query resources that support server-side search, to forward the query
     * to remote storage (for example using SEARCH feature on IMAP servers) and
     * merge their results with results from local index.
     *
     * This is useful especially when searching resources, that don't fetch full
     * payload by default, for example the IMAP resource, which only fetches headers
     * by default and the body is fetched on demand, which means that emails that
     * were not yet fully fetched cannot be indexed in local index, and thus cannot
     * be searched. With remote search, even those emails can be included in search
     * results.
     *
     * This feature is disabled by default.
     *
     * Results are streamed back to client as they are received from queried sources,
     * so this job can take some time to finish, but will deliver initial results
     * from local index fairly quickly.
     *
     * @param enabled Whether remote search is enabled
     * @since 4.13
     */
    void setRemoteSearchEnabled(bool enabled);

    /**
     * Returns whether remote search is enabled.
     *
     * @since 4.13
     */
    bool isRemoteSearchEnabled() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever new matching items have been fetched completely.
     *
     * @note This is an optimization, instead of waiting for the end of the job
     *       and calling items(), you can connect to this signal and get the items
     *       incrementally.
     *
     * @param items The matching items.
     */
    void itemsReceived(const Akonadi::Item::List &items);

protected:
    void doStart();
    virtual void doHandleResponse(const QByteArray &tag, const QByteArray &data);

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(ItemSearchJob)

    Q_PRIVATE_SLOT(d_func(), void timeout())
    //@endcond
};

}

#endif
