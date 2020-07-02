/*
    SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno.virlet@gmail.com>
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ENTITYMIMETYPEFILTERMODEL_H
#define AKONADI_ENTITYMIMETYPEFILTERMODEL_H

#include "akonadicore_export.h"
#include "entitytreemodel.h"

#include <QSortFilterProxyModel>

namespace Akonadi
{

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
    explicit EntityMimeTypeFilterModel(QObject *parent = nullptr);

    /**
     * Destroys the entity mime type filter model.
     */
    ~EntityMimeTypeFilterModel() override;

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
    Q_REQUIRED_RESULT QStringList mimeTypeInclusionFilters() const;

    /**
     * Returns the list of mime type exclusion filters.
     */
    Q_REQUIRED_RESULT QStringList mimeTypeExclusionFilters() const;

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

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Q_REQUIRED_RESULT bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    Q_REQUIRED_RESULT bool canFetchMore(const QModelIndex &parent) const override;

    Q_REQUIRED_RESULT QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const override;

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(EntityMimeTypeFilterModel)
    EntityMimeTypeFilterModelPrivate *const d_ptr;
    //@endcond
};

}

#endif
