/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_STATISTICSPROXYMODEL_H
#define AKONADI_STATISTICSPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>
#include "akonadi_next_export.h"
namespace Akonadi {

/**
 * @short A proxy model that exposes collection statistics through extra columns.
 *
 * This class can be used on top of an EntityTreeModel to display extra columns
 * summarizing statistics of collections.
 *
 * @code
 *
 *   Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel( ... );
 *
 *   Akonadi::StatisticsProxyModel *proxy = new Akonadi::StatisticsProxyModel();
 *   proxy->setSourceModel( model );
 *
 *   Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView( this );
 *   view->setModel( proxy );
 *
 * @endcode
 *
 * @author Kevin Ottens <ervin@kde.org>
 */
class AKONADI_NEXT_EXPORT StatisticsProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new statistics proxy model.
     *
     * @param parent The parent object.
     */
    explicit StatisticsProxyModel( QObject *parent = 0 );

    /**
     * Destroys the statistics proxy model.
     */
    virtual ~StatisticsProxyModel();

    virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    // QAbstractProxyModel does not proxy all methods...
    virtual bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );
    virtual QMimeData* mimeData( const QModelIndexList & indexes ) const;
    virtual QStringList mimeTypes() const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
