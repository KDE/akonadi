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

#ifndef AKONADI_RECURSIVEITEMFETCHJOB_H
#define AKONADI_RECURSIVEITEMFETCHJOB_H

#include "akonadicore_export.h"
#include "item.h"

#include <kjob.h>

namespace Akonadi {

class Collection;
class ItemFetchScope;

/**
 * @short Job that fetches all items of a collection recursive.
 *
 * This job takes a collection as argument and returns a list of
 * all items that are part of the passed collection and its child
 * collections. The items to fetch can be filtered by mime types and
 * the parts of the items that shall be fetched can
 * be specified via an ItemFetchScope.
 *
 * Example:
 *
 * @code
 *
 * // Assume the following Akonadi storage tree structure:
 * //
 * // Root Collection
 * // |
 * // +- Contacts
 * // |  |
 * // |  +- Private
 * // |  |
 * // |  `- Business
 * // |
 * // `- Events
 * //
 * // Collection 'Contacts' has the ID 15, then the following code
 * // returns all contact items from 'Contacts', 'Private' and 'Business'.
 *
 * const Akonadi::Collection contactsCollection( 15 );
 * const QStringList mimeTypes = QStringList() << KContacts::Addressee::mimeType();
 *
 * Akonadi::RecursiveItemFetchJob *job = new Akonadi::RecursiveItemFetchJob( contactsCollection, mimeTypes );
 * job->fetchScope().fetchFullPayload();
 * connect( job, SIGNAL(result(KJob*)), this, SLOT(fetchResult(KJob*)) );
 *
 * job->start();
 *
 * ...
 *
 * MyClass::fetchResult( KJob *job )
 * {
 *   Akonadi::RecursiveItemFetchJob *fetchJob = qobject_cast<Akonadi::RecursiveItemFetchJob*>( job );
 *   const Akonadi::Item::List items = fetchJob->items();
 *   // do something with the items
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.6
 */
class AKONADICORE_EXPORT RecursiveItemFetchJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new recursive item fetch job.
     *
     * @param collection The collection that shall be fetched recursive.
     * @param mimeTypes The list of mime types that will be used for filtering.
     * @param parent The parent object.
     */
    explicit RecursiveItemFetchJob(const Akonadi::Collection &collection,
                                   const QStringList &mimeTypes,
                                   QObject *parent = 0);

    /**
     * Destroys the recursive item fetch job.
     */
    ~RecursiveItemFetchJob();

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
    void setFetchScope(const Akonadi::ItemFetchScope &fetchScope);

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
    Akonadi::ItemFetchScope &fetchScope();

    /**
     * Returns the list of fetched items.
     */
    Akonadi::Item::List items() const;

    /**
     * Starts the recursive item fetch job.
     */
    virtual void start();

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void collectionFetchResult(KJob *))
    Q_PRIVATE_SLOT(d, void itemFetchResult(KJob *))
    //@endcond
};

}

#endif
