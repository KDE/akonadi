/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMFETCHJOB_H
#define AKONADI_ITEMFETCHJOB_H

#include <akonadi/item.h>
#include <akonadi/job.h>

namespace Akonadi {

class Collection;
class ItemFetchJobPrivate;
class ItemFetchScope;

/**
 * @short Job that fetches items from the Akonadi storage.
 *
 * This class is used to fetch items from the Akonadi storage.
 * Which parts of the items (e.g. headers only, attachments or all)
 * can be specified by the ItemFetchScope.
 *
 * Example:
 *
 * @code
 *
 * // Fetch all items with full payload from the root collection
 * Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( Akonadi::Collection::root() );
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( jobFinished( KJob* ) ) );
 * job->fetchScope().fetchFullPayload();
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
 *
 *   const Akonadi::Item::List items = fetchJob->items();
 *   foreach( const Akonadi::Item &item, items ) {
 *     qDebug() << "Item ID:" << item.id();
 *   }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT ItemFetchJob : public Job
{
    Q_OBJECT
  public:
    /**
     * Creates a new item fetch job that retrieves all items inside the given collection.
     *
     * @param collection The parent collection to fetch all items from.
     * @param parent The parent object.
     */
    explicit ItemFetchJob( const Collection &collection, QObject *parent = 0 );

    /**
     * Creates a new item fetch job that retrieves the specified item.
     * If the item has an uid set, this is used to identify the item on the Akonadi
     * server. If only a remote identifier is available, that one is used.
     * However, as remote identifier are not necessarily globally unique, you
     * need to specify the resource and/or collection to search in in that case,
     * using setCollection() or Akonadi::ResourceSelectJob.
     *
     * @param item The item to fetch.
     * @param parent The parent object.
     */
    explicit ItemFetchJob( const Item &item, QObject *parent = 0 );

    /**
     * Creates a new item fetch job that retrieves the specified items.
     * If the items have an uid set, this is used to identify the item on the Akonadi
     * server. If only a remote identifier is available, that one is used.
     * However, as remote identifier are not necessarily globally unique, you
     * need to specify the resource and/or collection to search in in that case,
     * using setCollection() or Akonadi::ResourceSelectJob.
     *
     * @param items The items to fetch.
     * @param parent The parent object.
     * @since 4.4
     */
    explicit ItemFetchJob( const Item::List &items, QObject *parent = 0 );

    /**
     * Destroys the item fetch job.
     */
    virtual ~ItemFetchJob();

    /**
     * Returns the fetched item.
     *
     * @note The items are invalid before the result( KJob* )
     *       signal has been emitted or if an error occurred.
     */
    Item::List items() const;

    /**
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an item's data is fetched
     * from the server, e.g. whether to fetch the full item payload or
     * only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     */
    void setFetchScope( ItemFetchScope &fetchScope ); // KDE5: remove

    /**
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an item's data is fetched
     * from the server, e.g. whether to fetch the full item payload or
     * only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     * @since 4.4
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
     * Specifies the collection the item is in.
     * This is only required when retrieving an item based on its remote id which might not be
     * unique globally.
     *
     * @see Akonadi::ResourceSelectJob
     */
    void setCollection( const Collection &collection );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever new items have been fetched completely.
     *
     * @note This is an optimization, instead of waiting for the end of the job
     *       and calling items(), you can connect to this signal and get the items
     *       incrementally.
     *
     * @param items The fetched items.
     */
    void itemsReceived( const Akonadi::Item::List &items );

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( ItemFetchJob )

    //@cond PRIVATE
    Q_PRIVATE_SLOT( d_func(), void selectDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void timeout() )
    //@endcond
};

}

#endif
