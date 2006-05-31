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
      @param parent The parent object.
    */
    CollectionModel( QObject *parent = 0 );

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

    /**
      Reimplemented.
    */
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    /**
      Reimplemented.
    */
    Qt::ItemFlags flags( const QModelIndex &index ) const;

    /**
      Add a new collection to the model and try to save it into the backend.
      @param parent The parent model index.
      @param name The name of the new collection.
      @returns false on immediate error.
    */
    bool createCollection( const QModelIndex &parent, const QString &name );

    /**
      Returns true if a new sub-collection for the given parent collection can be created.
      @param parent The parent model index.
    */
    bool canCreateCollection( const QModelIndex &parent ) const;

    /**
      Returns the collection path for the given model index.
      @param index The model index.
    */
    QByteArray pathForIndex( const QModelIndex &index ) const;

  private:
    /**
      Helper function to generate a model index for a given collection reference.
    */
    QModelIndex indexForPath( const QByteArray &path, int column = 0 );

  private slots:
    /**
      Removes a the given collection from the model.
    */
    void collectionRemoved( const QByteArray &path );

    /**
      Notify the model about collection changes.
    */
    void collectionChanged( const QByteArray &path );

    /**
      Connected to the collection fetch job.
    */
    void fetchDone( PIM::Job *job );

    /**
      Connected to the list jobs.
    */
    void listDone( PIM::Job *job );

    /**
      Connected to edit jobs.
    */
    void editDone( PIM::Job *job );

  private:
    class Private;
    Private *d;
};

}

#endif
