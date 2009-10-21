/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_PARTFETCHER_H
#define AKONADI_PARTFETCHER_H

#include <kjob.h>

#include "akonadi_export.h"

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
 * connect( fetcher, SIGNAL( result( KJob* ) ), SLOT( fetchResult( KJob* ) ) );
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
class AKONADI_EXPORT PartFetcher : public KJob
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
    PartFetcher( const QModelIndex &index, const QByteArray &partName, QObject *parent = 0 );

    /**
     * Starts the fetch operation.
     */
    virtual void start();

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
    //@cond PRIVATE
    Q_DECLARE_PRIVATE( Akonadi::PartFetcher )
    PartFetcherPrivate *const d_ptr;

    Q_PRIVATE_SLOT( d_func(), void fetchJobDone( KJob *job ) )
    //@endcond
};

}

#endif
