/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONMODEL_H
#define AKONADI_COLLECTIONMODEL_H

#include "libakonadi_export.h"

#include <libakonadi/collectionstatus.h>
#include <libakonadi/job.h>

#include <QtCore/QAbstractItemModel>

namespace Akonadi {

class Collection;

/**
  Model to handle a collection tree.

  @todo Split into generic and KDE dependent parts?
*/
class AKONADI_EXPORT CollectionModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    /**
      Extended item roles for collections.
    */
    enum CollectionItemRole {
      CollectionIdRole = Qt::UserRole, ///< The collection path.
      CollectionRole, ///< The actual collection object.
      ChildCreatableRole, ///< The collection can contain sub-collections.
      CollectionContentTypesRole, ///< Returns the mimetypes supported by the collection
      CollectionViewUserRole = Qt::UserRole + 32 ///< Role for user extensions
    };

    /**
      Create a new collection model.
      @param parent The parent object.
    */
    explicit CollectionModel( QObject *parent = 0 );

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
      Reimplemented.
    */
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    /**
      Reimplemented.
    */
    Qt::ItemFlags flags( const QModelIndex &index ) const;

    /**
      Reimplemented.
    */
    virtual Qt::DropActions supportedDropActions() const;

    /**
      Reimplemented.
    */
    virtual QStringList mimeTypes() const;

    /**
      Reimplemented.
     */
    virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;

    /**
      Reimplemented.
    */
   virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

  Q_SIGNALS:
    /**
     * emitted when the unread count of the complete model changes.
     */
    void unreadCountChanged( int );

  protected:
    /**
      Returns the collection for a given collection id.
      @param id A collection id.
    */
    Collection collectionForId( int id ) const;

    /**
      Enable fetching of collection status information.
      @see CollectionStatus.
    */
    void fetchCollectionStatus( bool enable );

    /**
      Also include unsubscribed collections.
    */
    void includeUnsubscribed( bool include = true );

  private:
    /**
      Helper function to generate a model index for a given collection reference.
    */
    QModelIndex indexForId( int id, int column = 0 );

    /**
      Helper method to remove a single row from the model (not from the Akonadi backend).
      @param row The row index.
      @param parent The parent model index.
    */
    bool removeRowFromModel( int row, const QModelIndex & parent = QModelIndex() );

    /**
      Returns true if a new sub-collection for the given parent collection can be created.
      @param parent The parent model index.
    */
    bool canCreateCollection( const QModelIndex &parent ) const;

    /**
      Returns wether the specified collection supports <em>any</em> of the given mime-types.
      @param index The model index.
      @param contentTypes The content types to check.
    */
    bool supportsContentType( const QModelIndex &index, const QStringList &contentTypes );

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void init() )
    Q_PRIVATE_SLOT( d, void collectionRemoved( int ) )
    Q_PRIVATE_SLOT( d, void collectionChanged( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d, void updateDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionStatusChanged( int, const Akonadi::CollectionStatus& ) )
    Q_PRIVATE_SLOT( d, void listDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void editDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void dropResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionsChanged( const Akonadi::Collection::List& ) )

};

}

#endif
