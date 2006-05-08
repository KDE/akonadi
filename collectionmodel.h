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

#ifndef PIM_COLLECTIONMODEL_H
#define PIM_COLLECTIONMODEL_H

#include <QAbstractItemModel>

#include <libakonadi/job.h>
#include <kdepim_export.h>

namespace PIM {

class Collection;

/**
  Model to handle a collection tree.

  @todo Add support for collection filtering, eg. deal only with collections
  containing contacts.

  @todo Split into generic and KDE dependent parts?
*/
class AKONADI_EXPORT CollectionModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    /**
      Create a new collection model.
      @param collections A hash containing all collections.
      The CollectionModel takes the ownership of the collection objects,
      ie. it will take care of deleting them.
      @param parent The parent object.
    */
    CollectionModel( const QHash<DataReference, Collection*> &collections,
                     QObject *parent = 0 );

    /**
      Destroys this model.
    */
    virtual ~CollectionModel();

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QModelIndex parent( const QModelIndex & index ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    /**
      Reimplemented from QAbstractItemModel.
    */
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    /**
      Reimplemented from QAbstractItemModel.

      Note: The removed collection is not deleted from the storage service,
      but only removed from the model.
    */
    virtual bool removeRows( int row, int count, const QModelIndex & parent = QModelIndex() );

  private:
    /**
      Process pending updates.
     */
    void processUpdates();

    /**
      Helper function to generate a model index for a given collection reference.
    */
    QModelIndex indexForReference( const DataReference &ref, int column = 0 );

    /**
      Removes a the given collection from the model.
    */
    void collectionRemoved( const DataReference &ref );

  private slots:
    /**
      Notify the model about collection changes.
    */
    void collectionChanged( const DataReference::List& references );

    /**
      Notify the model about removed collections.
    */
    void collectionRemoved( const DataReference::List& references );

    /**
      Connected to the collection fetch job.
    */
    void fetchDone( PIM::Job *job );

  private:
    class Private;
    Private *d;
};

}

#endif
