/*
  SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SELECTIONPROXYMODEL_H
#define AKONADI_SELECTIONPROXYMODEL_H

#include <KSelectionProxyModel>

#include "akonadicore_export.h"

namespace Akonadi
{

class SelectionProxyModelPrivate;

/**
 * @short A proxy model used to reference count selected Akonadi::Collection in a view
 *
 * Only selected Collections will be populated and monitored for changes. Unselected
 * Collections will be ignored.
 *
 * This model extends KSelectionProxyModel to implement reference counting on the Collections
 * in an EntityTreeModel. The EntityTreeModel must use LazyPopulation to enable
 * SelectionProxyModel to work.
 *
 * By selecting a Collection, its reference count will be increased. A Collection in the
 * EntityTreeModel which has a reference count of zero will ignore all signals from Monitor
 * about items changed, inserted, removed etc, which can be expensive operations.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * //                         itemView
 * //                             ^
 * //                             |
 * //                         itemModel
 * //                             |
 * //                         flatModel
 * //                             |
 * //   collectionView --> selectionModel
 * //           ^                 ^
 * //           |                 |
 * //  collectionFilter           |
 * //            \______________model
 *
 * EntityTreeModel *model = new EntityTreeModel( ... );
 *
 * // setup collection model
 * EntityMimeTypeFilterModel *collectionFilter = new EntityMimeTypeFilterModel( this );
 * collectionFilter->setSourceModel( model );
 * collectionFilter->addMimeTypeInclusionFilter( Collection::mimeType() );
 * collectionFilter->setHeaderGroup( EntityTreeModel::CollectionTreeHeaders );
 *
 * // setup collection view
 * EntityTreeView *collectionView = new EntityTreeView( this );
 * collectionView->setModel( collectionFilter );
 *
 * // setup selection model
 * SelectionProxyModel *selectionModel = new SelectionProxyModel( collectionView->selectionModel(), this );
 * selectionModel->setSourceModel( model );
 *
 * // setup item model
 * KDescendantsProxyModel *flatModel = new KDescendantsProxyModel( this );
 * flatModel->setSourceModel( selectionModel );
 *
 * EntityMimeTypeFilterModel *itemModel = new EntityMimeTypeFilterModel( this );
 * itemModel->setSourceModel( flatModel );
 * itemModel->setHeaderGroup( EntityTreeModel::ItemListHeaders );
 * itemModel->addMimeTypeExclusionFilter( Collection::mimeType() );
 *
 * EntityListView *itemView = new EntityListView( this );
 * itemView->setModel( itemModel );
 * @endcode
 *
 * See \ref libakonadi_integration "Integration in your Application" for further guidance on the use of this class.

 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADICORE_EXPORT SelectionProxyModel : public KSelectionProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new selection proxy model.
     *
     * @param selectionModel The selection model of the source view.
     * @param parent The parent object.
     */
    explicit SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent = nullptr);
    ~SelectionProxyModel();

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(SelectionProxyModel)
    SelectionProxyModelPrivate *const d_ptr;

    Q_PRIVATE_SLOT(d_func(), void rootIndexAdded(const QModelIndex &))
    Q_PRIVATE_SLOT(d_func(), void rootIndexAboutToBeRemoved(const QModelIndex &))
    //@endcond
};

}

#endif
