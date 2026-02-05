/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include "item.h"
#include "job.h"

namespace Akonadi
{
class TagFetchScope;
class ItemFetchScope;
class ItemSearchJobPrivate;
class SearchQuery;

/*!
 * \brief Job that searches for items in the Akonadi storage.
 *
 * This job searches for items that match a given search query and returns
 * the list of matching item.
 *
 * \code
 *
 * SearchQuery query;
 * query.addTerm(SearchTerm("From", "user1@domain.example", SearchTerm::CondEqual));
 * query.addTerm(SearchTerm("Date", QDateTime(QDate( 2014, 01, 27), QTime(00, 00, 00)), SearchTerm::CondGreaterThan);
 *
 * auto job = new Akonadi::ItemSearchJob(query);
 * job->fetchScope().fetchFullPayload();
 * connect(job, &Akonadi::ItemSearchJob::result, this, &MyClass::searchResult));
 *
 * ...
 *
 * MyClass::searchResult(KJob *job)
 * {
 *   auto searchJob = qobject_cast<Akonadi::ItemSearchJob*>(job);
 *   const Akonadi::Item::List items = searchJob->items();
 *   for (const Akonadi::Item &item : items) {
 *     // extract the payload and do further stuff
 *   }
 * }
 *
 * \endcode
 *
 * \author Tobias Koenig <tokoe@kde.org>
 * \since 4.4
 */
class AKONADICORE_EXPORT ItemSearchJob : public Job
{
    Q_OBJECT

public:
    /*!
     * Creates an invalid search job.
     *
     * \a parent The parent object.
     * \since 5.1
     */
    explicit ItemSearchJob(QObject *parent = nullptr);

    /*!
     * Creates an item search job.
     *
     * \a query The search query.
     * \a parent The parent object.
     * \since 4.13
     */
    explicit ItemSearchJob(const SearchQuery &query, QObject *parent = nullptr);

    /*!
     * Destroys the item search job.
     */
    ~ItemSearchJob() override;

    /*!
     * Sets the search \a query.
     *
     * \since 4.13
     */
    void setQuery(const SearchQuery &query);

    /*!
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an matching item's data is fetched
     * from the server, e.g. whether to fetch the full item payload or
     * only meta data.
     *
     * \a fetchScope The new scope for item fetch operations.
     *
     * \sa fetchScope()
     */
    void setFetchScope(const ItemFetchScope &fetchScope);

    /*!
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * Returns a reference to the current item fetch scope
     *
     * \sa setFetchScope() for replacing the current item fetch scope
     */
    ItemFetchScope &fetchScope();

    /*!
     * Sets the tag fetch scope.
     *
     * The tag fetch scope affects what scope of tags for each Item will be
     * retrieved.
     */
    void setTagFetchScope(const TagFetchScope &fetchScope);

    /*!
     * Returns the tag fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify
     * the current scope in-place.
     */
    TagFetchScope &tagFetchScope();

    /*!
     * Returns the items that matched the search query.
     */
    [[nodiscard]] Item::List items() const;

    /*!
     * Search only for items of given mime types.
     *
     * \since 4.13
     */
    void setMimeTypes(const QStringList &mimeTypes);

    /*!
     * Returns list of mime types to search in
     *
     * \since 4.13
     */
    [[nodiscard]] QStringList mimeTypes() const;

    /*!
     * Search only in given collections.
     *
     * When recursive search is enabled, all child collections of each specified
     * collection will be searched too
     *
     * By default all collections are be searched.
     *
     * \a collections Collections to search
     * \since 4.13
     */
    void setSearchCollections(const Collection::List &collections);

    /*!
     * Returns list of collections to search.
     *
     * This list does not include child collections that will be searched when
     * recursive search is enabled
     *
     * \since 4.13
     */
    [[nodiscard]] Collection::List searchCollections() const;

    /*!
     * Sets whether the search should recurse into collections
     *
     * When set to true, all child collections of the specific collections will
     * be search recursively.
     *
     * \a recursive Whether to search recursively
     * \since 4.13
     */
    void setRecursive(bool recursive);

    /*!
     * Returns whether the search is recursive
     *
     * \since 4.13
     */
    [[nodiscard]] bool isRecursive() const;

    /*!
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
     * \a enabled Whether remote search is enabled
     * \since 4.13
     */
    void setRemoteSearchEnabled(bool enabled);

    /*!
     * Returns whether remote search is enabled.
     *
     * \since 4.13
     */
    [[nodiscard]] bool isRemoteSearchEnabled() const;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever new matching items have been fetched completely.
     *
     * \
ote This is an optimization, instead of waiting for the end of the job
     *       and calling items(), you can connect to this signal and get the items
     *       incrementally.
     *
     * \a items The matching items.
     */
    void itemsReceived(const Akonadi::Item::List &items);

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(ItemSearchJob)

    Q_PRIVATE_SLOT(d_func(), void timeout())
};

}
