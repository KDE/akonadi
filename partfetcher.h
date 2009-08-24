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

#ifndef AKONADI_PART_FETCHER_H
#define AKONADI_PART_FETCHER_H

#include <QtCore/QObject>

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
 * and fetch them asyncronously from the Akonadi storage if necessary.
 *
 * The requested part is emitted though the partFetched signal.
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADI_EXPORT PartFetcher : public QObject
{
  Q_OBJECT

  public:
    /**
     * Creates a new part fetcher.
     *
     * @param parent The parent object.
     */
    explicit PartFetcher( QObject *parent = 0 );

    /**
     * Fetches the part called @p partName from the item at @p index
     */
    bool fetchPart( const QModelIndex &index, const QByteArray &partName );

    /**
     * @internal
     *
     * TODO: move to private class.
     */
    void reset();

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the requested part has been fetched successfully.
     *
     * @param index The index of the item the part was fetched from.
     * @param item The item the part was fetched from.
     * @param partName The name of the part that has been fetched.
     */
    void partFetched( const QModelIndex &index, const Item &item, const QByteArray &partName );
    void invalidated();

  private:
    Q_DECLARE_PRIVATE( Akonadi::PartFetcher )
    PartFetcherPrivate *d_ptr;

    Q_PRIVATE_SLOT( d_func(), void itemsFetched( const Akonadi::Item::List & ) )
    Q_PRIVATE_SLOT( d_func(), void fetchJobDone( KJob *job ) )
};

}

#endif
