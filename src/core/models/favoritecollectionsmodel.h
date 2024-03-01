/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"
#include <KSelectionProxyModel>

#include <memory>

class KConfigGroup;
class KJob;

namespace Akonadi
{
class EntityTreeModel;
class FavoriteCollectionsModelPrivate;

/**
 * @short A model that lists a set of favorite collections.
 *
 * In some applications you want to provide fast access to a list
 * of often used collections (e.g. Inboxes from different email accounts
 * in a mail application). Therefore you can use the FavoriteCollectionsModel
 * which stores the list of favorite collections in a given configuration
 * file.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * EntityTreeModel *sourceModel = new EntityTreeModel( ... );
 *
 * const KConfigGroup group = KGlobal::config()->group( "Favorite Collections" );
 *
 * FavoriteCollectionsModel *model = new FavoriteCollectionsModel( sourceModel, group, this );
 *
 * EntityListView *view = new EntityListView( this );
 * view->setModel( model );
 *
 * @endcode
 *
 * @author Kevin Ottens <ervin@kde.org>
 * @since 4.4
 */
class AKONADICORE_EXPORT FavoriteCollectionsModel : public KSelectionProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new favorite collections model.
     *
     * @param model The source model where the favorite collections
     *              come from.
     * @param group The config group that shall be used to save the
     *              selection of favorite collections.
     * @param parent The parent object.
     */
    FavoriteCollectionsModel(QAbstractItemModel *model, const KConfigGroup &group, QObject *parent = nullptr);

    /**
     * Destroys the favorite collections model.
     */
    ~FavoriteCollectionsModel() override;

    /**
     * Returns the list of favorite collections.
     * @deprecated Use collectionIds instead.
     */
    [[nodiscard]] AKONADICORE_DEPRECATED Collection::List collections() const;

    /**
     * Returns the list of ids of favorite collections set on the FavoriteCollectionsModel.
     *
     * Note that if you want Collections with actual data
     * you should use something like this instead:
     *
     * @code
     *   FavoriteCollectionsModel* favs = getFavsModel();
     *   Collection::List cols;
     *   const int rowCount = favs->rowCount();
     *   for (int row = 0; row < rowcount; ++row) {
     *     static const int column = 0;
     *     const QModelIndex index = favs->index(row, column);
     *     const Collection col = index.data(EntityTreeModel::CollectionRole).value<Collection>();
     *     cols << col;
     *   }
     * @endcode
     *
     * Note that due to the asynchronous nature of the model, this method returns collection ids
     * of collections which may not be in the model yet. If you want the ids of the collections
     * that are actually in the model, use a loop similar to above with the CollectionIdRole.
     */
    [[nodiscard]] QList<Collection::Id> collectionIds() const;

    /**
     * Return associate label for collection
     */
    [[nodiscard]] QString favoriteLabel(const Akonadi::Collection &col);
    [[nodiscard]] QString defaultFavoriteLabel(const Akonadi::Collection &col);

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

public Q_SLOTS:
    /**
     * Sets the @p collections as favorite collections.
     */
    void setCollections(const Akonadi::Collection::List &collections);

    /**
     * Adds a @p collection to the list of favorite collections.
     */
    void addCollection(const Akonadi::Collection &collection);

    /**
     * Removes a @p collection from the list of favorite collections.
     */
    void removeCollection(const Akonadi::Collection &collection);

    /**
     * Sets a custom @p label that will be used when showing the
     * favorite @p collection.
     */
    void setFavoriteLabel(const Akonadi::Collection &collection, const QString &label);

private:
    AKONADICORE_NO_EXPORT void pasteJobDone(KJob *job);
    /// @cond PRIVATE
    using KSelectionProxyModel::setSourceModel;

    std::unique_ptr<FavoriteCollectionsModelPrivate> const d;
    /// @endcond
};

}
