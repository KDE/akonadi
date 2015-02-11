/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>
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

#ifndef AKONADI_ENTITYMIMETYPEFILTERMODEL_H
#define AKONADI_ENTITYMIMETYPEFILTERMODEL_H

#include "akonadicore_export.h"
#include "entitytreemodel.h"

#include <QSortFilterProxyModel>

namespace Akonadi {

class EntityMimeTypeFilterModelPrivate;

/**
 * @short A proxy model that filters entities by mime type.
 *
 * This class can be used on top of an EntityTreeModel to exclude entities by mimetype
 * or to include only certain mimetypes.
 *
 * @code
 *
 *   Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel( this );
 *
 *   Akonadi::EntityMimeTypeFilterModel *proxy = new Akonadi::EntityMimeTypeFilterModel();
 *   proxy->addMimeTypeInclusionFilter( "message/rfc822" );
 *   proxy->setSourceModel( model );
 *
 *   Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView( this );
 *   view->setModel( proxy );
 *
 * @endcode
 *
 * @li If a mimetype is in both the exclusion list and the inclusion list, it is excluded.
 * @li If the mimeTypeInclusionFilter is empty, all mimetypes are
 *     accepted (except if they are in the exclusion filter of course).
 *
 *
 * @author Bruno Virlet <bruno.virlet@gmail.com>
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADICORE_EXPORT EntityMimeTypeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new entity mime type filter model.
     *
     * @param parent The parent object.
     */
    explicit EntityMimeTypeFilterModel(QObject *parent = Q_NULLPTR);

    /**
     * Destroys the entity mime type filter model.
     */
    virtual ~EntityMimeTypeFilterModel();

    /**
     * Add mime types to be shown by the filter.
     *
     * @param mimeTypes A list of mime types to be included.
     */
    void addMimeTypeInclusionFilters(const QStringList &mimeTypes);

    /**
     * Add mimetypes to filter out
     *
     * @param mimeTypes A list to exclude from the model.
     */
    void addMimeTypeExclusionFilters(const QStringList &mimeTypes);

    /**
     * Add mime type to be shown by the filter.
     *
     * @param mimeType A mime type to be shown.
     */
    void addMimeTypeInclusionFilter(const QString &mimeType);

    /**
     * Add mime type to be excluded by the filter.
     *
     * @param mimeType A mime type to be excluded.
     */
    void addMimeTypeExclusionFilter(const QString &mimeType);

    /**
     * Returns the list of mime type inclusion filters.
     */
    QStringList mimeTypeInclusionFilters() const;

    /**
     * Returns the list of mime type exclusion filters.
     */
    QStringList mimeTypeExclusionFilters() const;

    /**
     * Clear all mime type filters.
     */
    void clearFilters();

    /**
     * Sets the header @p set of the filter model.
     * @param headerGroup the header to set.
     * \sa EntityTreeModel::HeaderGroup
     */
    void setHeaderGroup(EntityTreeModel::HeaderGroup headerGroup);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    bool canFetchMore(const QModelIndex &parent) const Q_DECL_OVERRIDE;

    QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const Q_DECL_OVERRIDE;

    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(EntityMimeTypeFilterModel)
    EntityMimeTypeFilterModelPrivate *const d_ptr;
    //@endcond
};

}

#endif
