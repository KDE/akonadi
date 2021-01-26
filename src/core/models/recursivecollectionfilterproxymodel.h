/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>
    SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RECURSIVECOLLECTIONFILTERPROXYMODEL_H
#define AKONADI_RECURSIVECOLLECTIONFILTERPROXYMODEL_H

#include "akonadicore_export.h"

#include <QSortFilterProxyModel>

namespace Akonadi
{
class RecursiveCollectionFilterProxyModelPrivate;

/**
 * @short A model to filter out collections of non-matching content types.
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.6
 */
class AKONADICORE_EXPORT RecursiveCollectionFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new recursive collection filter proxy model.
     *
     * @param parent The parent object.
     */
    explicit RecursiveCollectionFilterProxyModel(QObject *parent = nullptr);

    /**
     * Destroys the recursive collection filter proxy model.
     */
    ~RecursiveCollectionFilterProxyModel() override;

    /**
     * Add content mime type to be shown by the filter.
     *
     * @param mimeType A mime type to be shown.
     */
    void addContentMimeTypeInclusionFilter(const QString &mimeType);

    /**
     * Add content mime types to be shown by the filter.
     *
     * @param mimeTypes A list of content mime types to be included.
     */
    void addContentMimeTypeInclusionFilters(const QStringList &mimeTypes);

    /**
     * Clears the current filters.
     */
    void clearFilters();

    /**
     * Replace the content mime types to be shown by the filter.
     *
     * @param mimeTypes A list of content mime types to be included.
     */
    void setContentMimeTypeInclusionFilters(const QStringList &mimeTypes);

    /**
     * Returns the currently included mimetypes in the filter.
     */
    Q_REQUIRED_RESULT QStringList contentMimeTypeInclusionFilters() const;

    /**
     * Add search pattern
     * @param pattern the search pattern to add
     * @since 4.8.1
     */
    void setSearchPattern(const QString &pattern);

    /**
     * Show only checked item
     * @param checked only shows checked item if set as @c true
     * @since 4.9
     */
    void setIncludeCheckedOnly(bool checked);

protected:
    int columnCount(const QModelIndex &index) const override;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

protected:
    RecursiveCollectionFilterProxyModelPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(RecursiveCollectionFilterProxyModel)
};

}

#endif
