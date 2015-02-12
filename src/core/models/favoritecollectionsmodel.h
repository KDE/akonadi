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

#ifndef AKONADI_FAVORITECOLLECTIONSMODEL_H
#define AKONADI_FAVORITECOLLECTIONSMODEL_H

#include "akonadicore_export.h"
#include "selectionproxymodel.h"
#include "collection.h"

class KConfigGroup;
class KJob;

namespace Akonadi {

class EntityTreeModel;

/**
 * @short A model that lists a set of favorite collections.
 *
 * In some applications you want to provide fast access to a list
 * of often used collections (e.g. Inboxes from different email accounts
 * in a mail application). Therefor you can use the FavoriteCollectionsModel
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
//TODO_KDE5: Make this a KRecursiveFilterProxyModel instead of a SelectionProxyModel
class AKONADICORE_EXPORT FavoriteCollectionsModel : public Akonadi::SelectionProxyModel
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
    FavoriteCollectionsModel(QAbstractItemModel *model, const KConfigGroup &group, QObject *parent = Q_NULLPTR);

    /**
     * Destroys the favorite collections model.
     */
    virtual ~FavoriteCollectionsModel();

    /**
     * Returns the list of favorite collections.
     * @deprecated Use collectionIds instead.
     */
    Collection::List collections() const;

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
    QList<Collection::Id> collectionIds() const;

    /**
     * Return associate label for collection
     */
    QString favoriteLabel(const Akonadi::Collection &col);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;
    QStringList mimeTypes() const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

public Q_SLOTS:
    /**
     * Sets the @p collections as favorite collections.
     */
    void setCollections(const Collection::List &collections);

    /**
     * Adds a @p collection to the list of favorite collections.
     */
    void addCollection(const Collection &collection);

    /**
     * Removes a @p collection from the list of favorite collections.
     */
    void removeCollection(const Collection &collection);

    /**
     * Sets a custom @p label that will be used when showing the
     * favorite @p collection.
     */
    void setFavoriteLabel(const Collection &collection, const QString &label);

private Q_SLOTS:
    void pasteJobDone(KJob *job);

private:
    //@cond PRIVATE
    using KSelectionProxyModel::setSourceModel;

    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void reload())
    Q_PRIVATE_SLOT(d, void rowsInserted(QModelIndex, int, int))
    //@endcond
};

}

#endif
