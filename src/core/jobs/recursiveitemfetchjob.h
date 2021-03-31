/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "item.h"

#include <KJob>

namespace Akonadi
{
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
    explicit RecursiveItemFetchJob(const Akonadi::Collection &collection, const QStringList &mimeTypes, QObject *parent = nullptr);

    /**
     * Destroys the recursive item fetch job.
     */
    ~RecursiveItemFetchJob() override;

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
    Q_REQUIRED_RESULT Akonadi::Item::List items() const;

    /**
     * Starts the recursive item fetch job.
     */
    void start() override;

private:
    /// @cond PRIVATE
    class Private;
    Private *const d;
    /// @endcond
};

}

