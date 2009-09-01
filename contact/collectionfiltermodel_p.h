/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef COLLECTIONFILTERMODEL_H
#define COLLECTIONFILTERMODEL_H

#include <QtGui/QSortFilterProxyModel>

#include <akonadi/collection.h>

/**
 * @short A filter model for Akonadi collections.
 *
 * This proxy model filters Akonadi collections by content
 * mime type and rights.
 * It works on a flat list of collections, so a EntityTreeModel
 * has to be adapted by using Akonadi::DescendantsProxyModel.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class CollectionFilterModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection filter model.
     *
     * @param parent The parent object.
     */
    CollectionFilterModel( QObject *parent );

    /**
     * Adds a mime type to the content mime type filter list.
     *
     * If a mime type has been added, only collections that contain
     * this mime type in their content mime types will be listed by
     * this proxy model.
     */
    void addContentMimeTypeFilter( const QString &mimeType );

    /**
     * Sets the collection @p rights filter.
     *
     * Only collections that allows this rights combination
     * will be listed by this proxy model.
     */
    void setRightsFilter( Akonadi::Collection::Rights rights );

  protected:
    virtual bool filterAcceptsRow( int row, const QModelIndex &parent ) const;

  private:
    QSet<QString> mContentMimeTypes;
    Akonadi::Collection::Rights mRights;
};

#endif
