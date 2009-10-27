/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef AKONADI_COLLECTIONFILTERPROXYMODEL_H
#define AKONADI_COLLECTIONFILTERPROXYMODEL_H

#include "akonadi_export.h"
#include <QtGui/QSortFilterProxyModel>

namespace Akonadi {

class CollectionModel;

/**
 * @short A proxy model that filters collections by mime type.
 *
 * This class can be used on top of a CollectionModel to filter out
 * all collections that doesn't match a given mime type.
 *
 * For instance, a mail application will use addMimeType( "message/rfc822" ) to only show
 * collections containing mail.
 *
 * @code
 *
 *   Akonadi::CollectionModel *model = new Akonadi::CollectionModel( this );
 *
 *   Akonadi::CollectionFilterProxyModel *proxy = new Akonadi::CollectionFilterProxyModel();
 *   proxy->addMimeTypeFilter( "message/rfc822" );
 *   proxy->setSourceModel( model );
 *
 *   QTreeView *view = new QTreeView( this );
 *   view->setModel( proxy );
 *
 * @endcode
 *
 * @author Bruno Virlet <bruno.virlet@gmail.com>
 */
class AKONADI_EXPORT CollectionFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection proxy filter model.
     *
     * @param parent The parent object.
     */
    explicit CollectionFilterProxyModel( QObject *parent = 0 );

    /**
     * Destroys the collection proxy filter model.
     */
    virtual ~CollectionFilterProxyModel();

    /**
     * Add mime types to be shown by the filter.
     *
     * @param mimeTypes A list of mime types to be shown.
     */
    void addMimeTypeFilters( const QStringList &mimeTypes );

    /**
     * Add mime type to be shown by the filter.
     *
     * @param mimeType A mime type to be shown.
     */
    void addMimeTypeFilter( const QString &mimeType );

    /**
     * Returns the list of mime type filters.
     */
    QStringList mimeTypeFilters() const;

    /**
     * Clear all mime type filters.
     */
    void clearFilters();

    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

  protected:
    virtual bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
