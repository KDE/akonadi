/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#ifndef AKONADI_MESSAGECOLLECTIONMODEL_H
#define AKONADI_MESSAGECOLLECTIONMODEL_H

#include <libakonadi/collectionmodel.h>
#include <kdepim_export.h>

namespace Akonadi {

/**
  Extended modell for message collections.
  Supports columns for message unread/total counts.
*/
class AKONADI_EXPORT MessageCollectionModel : public CollectionModel
{
  Q_OBJECT

  public:
    /**
      Create a new message collection model.
      @param parent The parent object.
    */
    MessageCollectionModel( QObject *parent = 0 );

    /**
      Reimplemented from CollectionModel.
    */
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from CollectionModel.
    */
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  private:
    class Private;
    Private *d;

};

}

#endif
