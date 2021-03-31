/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KJob>

#include "akonadicore_export.h"

class QModelIndex;

namespace Akonadi
{
class Item;
class PartFetcherPrivate;

/**
 * @short Convenience class for getting payload parts from an Akonadi Model.
 *
 * This class can be used to retrieve individual payload parts from an EntityTreeModel,
 * and fetch them asynchronously from the Akonadi storage if necessary.
 *
 * The requested part is emitted though the partFetched signal.
 *
 * Example:
 *
 * @code
 *
 * const QModelIndex index = view->selectionModel()->currentIndex();
 *
 * PartFetcher *fetcher = new PartFetcher( index, Akonadi::MessagePart::Envelope );
 * connect( fetcher, SIGNAL(result(KJob*)), SLOT(fetchResult(KJob*)) );
 * fetcher->start();
 *
 * ...
 *
 * MyClass::fetchResult( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << job->errorText();
 *     return;
 *   }
 *
 *   PartFetcher *fetcher = qobject_cast<PartFetcher*>( job );
 *
 *   const Item item = fetcher->item();
 *   // do something with the item
 * }
 *
 * @endcode
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADICORE_EXPORT PartFetcher : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new part fetcher.
     *
     * @param index The index of the item to fetch the part from.
     * @param partName The name of the payload part to fetch.
     * @param parent The parent object.
     */
    PartFetcher(const QModelIndex &index, const QByteArray &partName, QObject *parent = nullptr);

    /**
     * Destroys the part fetcher.
     */
    ~PartFetcher() override;

    /**
     * Starts the fetch operation.
     */
    void start() override;

    /**
     * Returns the index of the item the part was fetched from.
     */
    QModelIndex index() const;

    /**
     * Returns the name of the part that has been fetched.
     */
    QByteArray partName() const;

    /**
     * Returns the item that contains the fetched payload part.
     */
    Item item() const;

private:
    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(Akonadi::PartFetcher)
    PartFetcherPrivate *const d_ptr;

    /// @endcond
};

}

