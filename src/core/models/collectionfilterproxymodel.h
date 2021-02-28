/*
    SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno.virlet@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONFILTERPROXYMODEL_H
#define AKONADI_COLLECTIONFILTERPROXYMODEL_H

#include "akonadicore_export.h"
#include <QSortFilterProxyModel>

namespace Akonadi
{
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
class AKONADICORE_EXPORT CollectionFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new collection proxy filter model.
     *
     * @param parent The parent object.
     */
    explicit CollectionFilterProxyModel(QObject *parent = nullptr);

    /**
     * Destroys the collection proxy filter model.
     */
    ~CollectionFilterProxyModel() override;

    /**
     * Adds a list of mime types to be shown by the filter.
     *
     * @param mimeTypes A list of mime types to be shown.
     */
    void addMimeTypeFilters(const QStringList &mimeTypes);

    /**
     * Adds a mime type to be shown by the filter.
     *
     * @param mimeType A mime type to be shown.
     */
    void addMimeTypeFilter(const QString &mimeType);

    /**
     * Returns the list of mime type filters.
     */
    Q_REQUIRED_RESULT QStringList mimeTypeFilters() const;

    /**
     * Sets whether we want virtual collections to be filtered or not.
     * By default, virtual collections are accepted.
     *
     * @param exclude If true, virtual collections aren't accepted.
     *
     * @since 4.7
     */
    void setExcludeVirtualCollections(bool exclude);
    /*
     * @since 4.12
     */
    Q_REQUIRED_RESULT bool excludeVirtualCollections() const;

    /**
     * Clears all mime type filters.
     */
    void clearFilters();

    Q_REQUIRED_RESULT Qt::ItemFlags flags(const QModelIndex &index) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    /// @cond PRIVATE
    class Private;
    Private *const d;
    /// @endcond
};

}

#endif
