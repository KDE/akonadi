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

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class ItemFetchScope;
class ItemSearchJobPrivate;

/**
 * @short Job that searches for items in the Akonadi storage.
 *
 * This job searches for items that match a given search query and returns
 * the list of matching item.
 *
 * @code
 *
 * const QString query = "..."; // some sparql query
 *
 * Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( query );
 * job->fetchScope().fetchFullPayload();
 * connect( job, SIGNAL( result( KJob* ) ), this, SLOT( searchResult( KJob* ) ) );
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
class AKONADI_EXPORT ItemSearchJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates an item search job.
     *
     * @param query The search query in SPARQL format.
     * @param parent The parent object.
     */
    explicit ItemSearchJob( const QString &query, QObject *parent = 0 );

    /**
     * Destroys the item search job.
     */
    ~ItemSearchJob();

    /**
     * Sets the search @p query in SPARQL format.
     */
    void setQuery( const QString &query );

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
    void setFetchScope( const ItemFetchScope &fetchScope );

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
    void itemsReceived( const Akonadi::Item::List &items );

  protected:
    void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE( ItemSearchJob )

    Q_PRIVATE_SLOT( d_func(), void timeout() )
    //@endcond
};

}

#endif
