/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_ENTITYTREEMODEL_H
#define AKONADI_ENTITYTREEMODEL_H


#include <QtCore/QAbstractItemModel>
#include <QtCore/QStringList>

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include "akonadi_next_export.h"

// TODO (Applies to all these 'new' models, not just EntityTreeModel):
// * Figure out how LazyPopulation and signals from monitor containing items should
//     fit together. Possibly store a list of collections whose items have already
//     been lazily fetched.
// * Fgure out whether DescendantEntitiesProxyModel needs to use fetchMore.
// * Profile this and DescendantEntitiesProxyModel. Make sure it's faster than
//     FlatCollectionProxyModel. See if the cache in that class can be cleared less often.
// * Unit tests. Much of the stuff here is not covered by modeltest, and some of
//     it is akonadi specific, such as setting root collection etc.
// * Implement support for includeUnsubscribed.
// * Use CollectionStatistics for item count stuff. Find out if I can get stats by mimetype.
// * Make sure there are applications using it before committing to it until KDE5.
//     Some API/ virtual methods might need to be added when real applications are made.
// * Implement ordering support.
// * Implement some proxy models for time-table like uses, eg KOrganizer events.
// * Apidox++

namespace Akonadi
{
class Item;
class CollectionStatistics;
class Monitor;
class Session;
class ItemFetchScope;

class EntityTreeModelPrivate;

/**
 * @short A model for collections and items together.
 *
 * This class is a wrapper around a Akonadi::Monitor object. The model represents a
 * part of the collection and item tree configured in the Monitor.
 *
 * @code
 *
 *   Monitor *monitor = new Monitor(this);
 *   monitor->setCollectionMonitored(Collection::root());
 *   monitor->setMimeTypeMonitored(KABC::addresseeMimeType());
 *
 *   EntityTreeModel *model = new EntityTreeModel( session, monitor, this );
 *
 *   EntityTreeView *view = new EntityTreeView( this );
 *   view->setModel( model );
 *
 * @endcode
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADI_NEXT_EXPORT EntityTreeModel : public QAbstractItemModel
{
  Q_OBJECT

  public:
    /**
     * Describes the roles for items. Roles for collections are defined by the superclass.
     */
    enum Roles {
      //sebsauer, 2009-05-07; to be able here to keep the akonadi_next EntityTreeModel compatible with
      //the akonadi_old ItemModel and CollectionModel, we need to use the same int-values for
      //ItemRole, ItemIdRole and MimeTypeRole like the Akonadi::ItemModel is using and the same
      //CollectionIdRole and CollectionRole like the Akonadi::CollectionModel is using.
      ItemIdRole = Qt::UserRole + 1,          ///< The item id
      ItemRole = Qt::UserRole + 2,            ///< The Item
      MimeTypeRole = Qt::UserRole + 3,        ///< The mimetype of the entity

      CollectionIdRole = Qt::UserRole + 10,   ///< The collection id.
      CollectionRole = Qt::UserRole + 11,     ///< The collection.

      RemoteIdRole,                           ///< The remoteId of the entity
      CollectionChildOrderRole,               ///< Ordered list of child items if available
      AmazingCompletionRole,                  ///< Role used to implement amazing completion
      ParentCollection,                       ///< The parent collection of the entity
      UserRole = Qt::UserRole + 1000,         ///< Role for user extensions.
      TerminalUserRole = 10000                ///< Last role for user extensions. Don't use a role beyond this or headerData will break.
    };

    /**
     * Describes what header information the model shall return.
     */
    enum HeaderGroup {
      EntityTreeHeaders,      ///< Header information for a tree with collections and items
      CollectionTreeHeaders,  ///< Header information for a collection-only tree
      ItemListHeaders,        ///< Header information for a list of items
      UserHeaders = 1000      ///< Last header information for submodel extensions
    };

    /**
     * Creates a new entity tree model.
     *
     * @param session The Session to use to communicate with Akonadi.
     * @param monitor The Monitor whose entities should be represented in the model.
     * @param parent The parent object.
     */
    EntityTreeModel( Session *session, Monitor *monitor, QObject *parent = 0 );

    /**
     * Destroys the entity tree model.
     */
    virtual ~EntityTreeModel();

    /**
     * Describes how the model should populated its items.
     */
    enum ItemPopulationStrategy {
      NoItemPopulation,    ///< Do not include items in the model.
      ImmediatePopulation, ///< Retrieve items immediately when their parent is in the model. This is the default.
      LazyPopulation       ///< Fetch items only when requested (using canFetchMore/fetchMore)
    };

    /**
     * Sets the item population @p strategy of the model.
     */
    void setItemPopulationStrategy( ItemPopulationStrategy strategy );

    /**
     * Returns the item population strategy of the model.
     */
    ItemPopulationStrategy itemPopulationStrategy() const;

    /**
     * Sets the root collection to create an entity tree for.
     * The @p collection must be a valid Collection object.
     *
     * By default the Collection::root() is used.
     */
    void setRootCollection( const Collection &collection );

    /**
     * Returns the root collection of the entity tree.
     */
    Collection rootCollection() const;

    /**
     * Sets whether the root collection shall be provided by the model.
     *
     * @see setRootCollectionDisplayName()
     */
    void setIncludeRootCollection( bool include );

    /**
     * Returns whether the root collection is provided by the model.
     */
    bool includeRootCollection() const;

    /**
     * Sets the display @p name of the root collection of the model.
     * The default display name is "[*]".
     *
     * @note The display name for the root collection is only used if
     *       the root collection has been included with setIncludeRootCollection().
     */
    void setRootCollectionDisplayName( const QString &name );

    /**
     * Returns the display name of the root collection.
     */
    QString rootCollectionDisplayName() const;

    /**
     * Describes what collections shall be fetched by and represent in the model.
     */
    enum CollectionFetchStrategy {
      FetchNoCollections,               ///< Fetches nothing. This creates an empty model.
      FetchFirstLevelChildCollections,  ///< Fetches first level collections in the root collection.
      FetchCollectionsRecursive         ///< Fetches collections in the root collection recursively. This is the default.
    };

    /**
     * Sets the collection fetch @p strategy of the model.
     */
    void setCollectionFetchStrategy( CollectionFetchStrategy strategy );

    /**
     * Returns the collection fetch strategy of the model.
     */
    CollectionFetchStrategy collectionFetchStrategy() const;

    /**
     * Returns the model index for the given @p collection.
     */
    QModelIndex indexForCollection( const Collection &collection ) const;

    /**
     * Returns the model indexes for the given @p item.
     */
    QModelIndexList indexesForItem( const Item &item ) const;

    /**
     * Returns the collection for the given collection @p id.
     */
    Collection collectionForId( Collection::Id id ) const;

    /**
     * Returns the item for the given item @p id.
     */
    Item itemForId( Item::Id id ) const;

    // TODO: Remove these and use the Monitor instead. Need to add api to Monitor for this.
    void setIncludeUnsubscribed( bool include );
    bool includeUnsubscribed() const;

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    virtual QStringList mimeTypes() const;

    virtual Qt::DropActions supportedDropActions() const;
    virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex & index ) const;

    // TODO: Review the implementations of these. I think they could be better.
    virtual bool canFetchMore( const QModelIndex & parent ) const;
    virtual void fetchMore( const QModelIndex & parent );
    virtual bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;

    /**
    Reimplemented to handle the AmazingCompletionRole.
    */
    virtual QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap ) ) const;

    /**
      Reimplement this in a subclass to return true if @p item matches @p value with @p flags in the AmazingCompletionRole.
    */
    virtual bool match(const Item &item, const QVariant &value, Qt::MatchFlags flags) const;

    /**
      Reimplement this in a subclass to return true if @p collection matches @p value with @p flags in the AmazingCompletionRole.
    */
    virtual bool match(const Collection &collection, const QVariant &value, Qt::MatchFlags flags) const;

    
  protected:
    /**
     * Clears and resets the model. Always call this instead of the reset method in the superclass.
     * Using the reset method will not reliably clear or refill the model.
     */
    void clearAndReset();

    /**
     * Provided for convenience of subclasses.
     */
    virtual QVariant getData( const Item &item, int column, int role = Qt::DisplayRole ) const;

    /**
     * Provided for convenience of subclasses.
     */
    virtual QVariant getData( const Collection &collection, int column, int role = Qt::DisplayRole ) const;

    /**
     * Reimplement this to provide different header data. This is needed when using one model
     * with multiple proxies and views, and each should show different header data.
     */
    virtual QVariant getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const;

    /**
     * Removes the rows from @p start to @p end from @parent
     */
    virtual bool removeRows( int start, int end, const QModelIndex &parent = QModelIndex() );

  private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE( EntityTreeModel )
    EntityTreeModelPrivate *d_ptr;

    // Make these private, they shouldn't be called by applications
    virtual bool insertRows( int , int, const QModelIndex& = QModelIndex() );
    virtual bool insertColumns( int, int, const QModelIndex& = QModelIndex() );
    virtual bool removeColumns( int, int, const QModelIndex& = QModelIndex() );

    Q_PRIVATE_SLOT( d_func(), void monitoredCollectionStatisticsChanged( Akonadi::Collection::Id,
                                                                         const Akonadi::CollectionStatistics& ) )

    Q_PRIVATE_SLOT( d_func(), void startFirstListJob() )
    // Q_PRIVATE_SLOT( d_func(), void slotModelReset() )

    // TODO: Can I merge these into one jobResult slot?
    Q_PRIVATE_SLOT( d_func(), void fetchJobDone( KJob *job ) )
    Q_PRIVATE_SLOT( d_func(), void copyJobDone( KJob *job ) )
    Q_PRIVATE_SLOT( d_func(), void moveJobDone( KJob *job ) )
    Q_PRIVATE_SLOT( d_func(), void updateJobDone( KJob *job ) )

    Q_PRIVATE_SLOT( d_func(), void itemsFetched( Akonadi::Item::List ) )
    Q_PRIVATE_SLOT( d_func(), void collectionsFetched( Akonadi::Collection::List ) )
    Q_PRIVATE_SLOT( d_func(), void ancestorsFetched( Akonadi::Collection::List ) )

    Q_PRIVATE_SLOT( d_func(), void monitoredMimeTypeChanged( const QString&, bool ) )

    Q_PRIVATE_SLOT( d_func(), void monitoredCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredCollectionRemoved( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredCollectionChanged( const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredCollectionMoved( const Akonadi::Collection&, const Akonadi::Collection&,
                                                             const Akonadi::Collection&) )

    Q_PRIVATE_SLOT( d_func(), void monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredItemRemoved( const Akonadi::Item& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredItemMoved( const Akonadi::Item&, const Akonadi::Collection&,
                                                       const Akonadi::Collection& ) )

    Q_PRIVATE_SLOT( d_func(), void monitoredItemLinked( const Akonadi::Item&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void monitoredItemUnlinked( const Akonadi::Item&, const Akonadi::Collection& ) )
    //@endcond
};

} // namespace

#endif
