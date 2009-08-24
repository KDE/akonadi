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

#include <QObject>
#include <QModelIndex>

#include "item.h"
#include "akonadi_export.h"

namespace Akonadi
{

class PartFetcherPrivate;

/**
  A convenience class for getting individual payload parts from the model, and fetching asyncronously from
  Akonadi if necessary.

  The requested part is emitted though the partFetched signal.

  @author Stephen Kelly
  @since 4.4
*/
class AKONADI_EXPORT PartFetcher : public QObject
{
  Q_OBJECT
public:
  explicit PartFetcher( QObject *parent = 0 );

  /**
    Fetch the part called @p partname from the item at index @p idx.
  */
  bool fetchPart( const QModelIndex &idx, const QByteArray &partName );

  void reset();

signals:
  void partFetched( const QModelIndex &idx, const Item &item, const QByteArray &partName );
  void invalidated();

private:
  Q_DECLARE_PRIVATE( Akonadi::PartFetcher )
  PartFetcherPrivate *d_ptr;

  Q_PRIVATE_SLOT( d_func(), void itemsFetched( const Akonadi::Item::List & ) )
  Q_PRIVATE_SLOT( d_func(), void fetchJobDone( KJob *job ) )

};

}

#endif
