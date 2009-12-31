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

#include "akonadi_export.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QStringList>

namespace Akonadi
{

class ChangeRecorder;
class CollectionStatistics;
class Item;
class ItemFetchScope;
class Monitor;
class Session;

class EntityTreeModelPrivate;

/**
 * @short A model for collections and items together.
 *
 * Akonadi models and views provide a high level way to interact with the akonadi server.
 * Most applications will use these classes.
 *
 * Models provide an interface for viewing, deleting and moving Items and Collections.
 * Additionally, the models are updated automatically if another application changes the
 * data or inserts of deletes items etc.
 *
 * @note The EntityTreeModel should be used with the EntityTreeView or the EntityListView class
 * either directly or indirectly via proxy models.
 *
 * <h3>Retrieving Collections and Items from the model</h3>
 *
 * If you want to retrieve and Item or Collection from the model, and already have a valid
 * QModelIndex for the correct row, the Collection can be retrieved like this:
 *
 * @code
 * Collection col = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
 * @endcode
 *
 * And similarly for Items. This works even if there is a proxy model between the calling code
 * and the EntityTreeModel.
 *
 * If you want to retrieve a Collection for a particular Collection::Id  and you do not yet
 * have a valid QModelIndex, use match:
 *
 * @code
 * QModelIndexList list = model->match( QModelIndex(), CollectionIdRole, id );
 * if ( list.isEmpty() )
 *   return; // A Collection with that Id is not in the model.
 * Q_ASSERT( list.size() == 1 ); // Otherwise there must be only one instance of it.
 * Collection collection = list.at( 0 ).data( EntityTreeModel::CollectionRole ).value<Collection>();
 * @endcode
 *
 * Not that a single Item may appear multiple times in a model, so the list size may not be 1
 * if it is not empty in that case, so the Q_ASSERT should not be used.
 * @see virtual-collections
 *
 * <h3>Using EntityTreeModel in your application</h3>
 *
 * The responsibilities which fall to the application developer are
 * - Configuring the ChangeRecorder and EntityTreeModel
 * - Making use of this class via proxy models
 * - Subclassing for type specific display information
 *
 * <h3>Creating and configuring the EntityTreeModel</h3>
 *
 * This class is a wrapper around a Akonadi::ChangeRecorder object. The model represents a
 * part of the collection and item tree configured in the ChangeRecorder. The structure of the
 * model mirrors the structure of Collections and Items on the %Akonadi server.
 *
 * The following code creates a model which fetches items and collections relevant to
 * addressees (contacts), and automatically manages keeping the items up to date.
 *
 * @code
 *
 *   ChangeRecorder *changeRecorder = new ChangeRecorder( this );
 *   changeRecorder->setCollectionMonitored( Collection::root() );
 *   changeRecorder->setMimeTypeMonitored( KABC::addresseeMimeType() );
 *   changeRecorder->setSession( session );
 *
 *   EntityTreeModel *model = new EntityTreeModel( changeRecorder, this );
 *
 *   EntityTreeView *view = new EntityTreeView( this );
 *   view->setModel( model );
 *
 * @endcode
 *
 * The EntityTreeModel will show items of a different type by changing the line
 *
 * @code
 * changeRecorder->setMimeTypeMonitored( KABC::addresseeMimeType() );
 * @endcode
 *
 * to a different mimetype. KABC::addresseeMimeType() is an alias for "text/directory". If changed to KMime::Message::mimeType()
 * (an alias for "message/rfc822") the model would instead contain emails. The model can be configured to contain items of any mimetype
 * known to %Akonadi.
 *
 * @note The EntityTreeModel does some extra configuration on the Monitor, such as setting itemFetchScope() and collectionFetchScope()
 * to retrieve all ancestors. This is necessary for proper function of the model.
 *
 * @see Akonadi::ItemFetchScope::AncestorRetrieval.
 *
 * @see akonadi-mimetypes.
 *
 * The EntityTreeModel can be further configured for certain behaviours such as fetching of collections and items.
 *
 * The model can be configured to not fetch items into the model (ie, fetch collections only) by setting
 *
 * @code
 * entityTreeModel->setItemPopulationStrategy( EntityTreeModel::NoItemPopulation );
 * @endcode
 *
 * The items may be fetched lazily, i.e. not inserted into the model until request by the user for performance reasons.
 *
 * The Collection tree is always built immediately if Collections are to be fetched.
 *
 * @code
 * entityTreeModel->setItemPopulationStrategy( EntityTreeModel::LazyPopulation );
 * @endcode
 *
 * This will typically be used with a EntityMimeTypeFilterModel in a configuration such as KMail4.5 or AkonadiConsole.
 *
 * The CollectionFetchStrategy determines how the model will be populated with Collections. That is, if FetchNoCollections is set,
 * no collections beyond the root of the model will be fetched. This can be used in combination with setting a particular Collection to monitor.
 *
 * @code
 * // Get an collection id from a config file.
 * Collection::Id id;
 * monitor->setCollectionMonitored( Collection( id ) );
 * // ... Other initialization code.
 * entityTree->setCollectionFetchStrategy( FetchNoCollections );
 * @endcode
 *
 * This has the effect of creating a model of only a list of Items, and not collections. This is similar in behaviour and aims to the ItemModel.
 * By using FetchFirstLevelCollections instead, a mixed list of entities can be created.
 *
 * @note It is important that you set only one Collection to be monitored in the monitor object. This one collection will be the root of the tree.
 * If you need a model with a more complex structure, consider monitoring a common ancestor and using a SelectionProxyModel.
 *
 * @see lazy-model-population
 *
 * It is also possible to show the root Collection as part of the selectable model:
 *
 * @code
 * entityTree->setIncludeRootCollection( true );
 * @endcode
 *
 *
 * By default the displayed name of the root collection is '[*]', because it doesn't require i18n, and is generic. It can be changed too.
 *
 * @code
 * entityTree->setIncludeRootCollection( true );
 * entityTree->setRootCollectionDisplayName( i18nc( "Name of top level for all addressbooks in the application", "[All AddressBooks]" ) )
 * @endcode
 *
 * This feature is used in KAddressBook.
 *
 * <h2>Using EntityTreeModel with Proxy models</h2>
 *
 * An Akonadi::SelectionProxyModel can be used to simplify managing selection in one view through multiple proxy models to a representation in another view.
 * The selectionModel of the initial view is used to create a proxied model which filters out anything not related to the current selection.
 *
 * @code
 * // ... create an EntityTreeModel
 *
 * collectionTree = new EntityMimeTypeFilterModel( this );
 * collectionTree->setSourceModel( entityTreeModel );
 *
 * // Include only collections in this proxy model.
 * collectionTree->addMimeTypeInclusionFilter( Collection::mimeType() );
 * collectionTree->setHeaderGroup( EntityTreeModel::CollectionTreeHeaders );
 *
 * treeview->setModel(collectionTree);
 *
 * // SelectionProxyModel can handle complex selections:
 * treeview->setSelectionMode( QAbstractItemView::ExtendedSelection );
 *
 * SelectionProxyModel *selProxy = new SelectionProxyModel( treeview->selectionModel(), this );
 * selProxy->setSourceModel( entityTreeModel );
 *
 * itemList = new EntityMimeTypeFilterModel( this );
 * itemList->setSourceModel( selProxy );
 *
 * // Filter out collections. Show only items.
 * itemList->addMimeTypeExclusionFilter( Collection::mimeType() );
 * itemList->setHeaderGroup( EntityTreeModel::ItemListHeaders );
 *
 * EntityTreeView *itemView = new EntityTreeView( splitter );
 * itemView->setModel( itemList );
 * @endcode
 *
 * The SelectionProxyModel can handle complex selections.
 *
 * See the KSelectionProxyModel documentation for the valid configurations of a Akonadi::SelectionProxyModel.
 *
 * Obviously, the SelectionProxyModel may be used in a view, or further processed with other proxy models. Typically, the result
 * from this model will be further filtered to remove collections from the item list as in the above example.
 *
 * There are several advantages of using EntityTreeModel with the SelectionProxyModel, namely the items can be fetched and cached
 * instead of being fetched many times, and the chain of proxies from the core model to the view is automatically handled. There is
 * no need to manage all the mapToSource and mapFromSource calls manually.
 *
 * A KDescendantsProxyModel can be used to represent all descendants of a model as a flat list.
 * For example, to show all descendant items in a selected Collection in a list:
 * @code
 * collectionTree = new EntityMimeTypeFilterModel( this );
 * collectionTree->setSourceModel( entityTreeModel );
 *
 * // Include only collections in this proxy model.
 * collectionTree->addMimeTypeInclusionFilter( Collection::mimeType() );
 * collectionTree->setHeaderGroup( EntityTreeModel::CollectionTreeHeaders );
 *
 * treeview->setModel( collectionTree );
 *
 * SelectionProxyModel *selProxy = new SelectionProxyModel( treeview->selectionModel(), this );
 * selProxy->setSourceModel( entityTreeModel );
 *
 * descendedList = new DescendantEntitiesProxyModel( this );
 * descendedList->setSourceModel( selProxy );
 *
 * itemList = new EntityMimeTypeFilterModel( this );
 * itemList->setSourceModel( descendedList );
 *
 * // Exclude collections from the list view.
 * itemList->addMimeTypeExclusionFilter( Collection::mimeType() );
 * itemList->setHeaderGroup( EntityTreeModel::ItemListHeaders );
 *
 * listView = new EntityTreeView( this );
 * listView->setModel( itemList );
 * @endcode
 *
 *
 * Note that it is important in this case to use the DescendantEntitesProxyModel before the EntityMimeTypeFilterModel.
 * Otherwise, by filtering out the collections first, you would also be filtering out their child items.
 *
 * This pattern is used in KAddressBook.
 *
 * It would not make sense to use a KDescendantsProxyModel with LazyPopulation.
 *
 * <h3>Subclassing EntityTreeModel</h3>
 *
 * Usually an application will create a subclass of an EntityTreeModel and use that in several views via proxy models.
 *
 * The subclassing is necessary in order for the data in the model to have type-specific representation in applications
 *
 * For example, the headerData for an EntityTreeModel will be different depending on whether it is in a view showing only Collections
 * in which case the header data should be "AddressBooks" for example, or only Items, in which case the headerData would be
 * for example "Family Name", "Given Name" and "Email addres" for contacts or "Subject", "Sender", "Date" in the case of emails.
 *
 * Additionally, the actual data shown in the rows of the model should be type specific.
 *
 * In summary, it must be possible to have different numbers of columns, different data in hte rows of those columns, and different
 * titles for each column depending on the contents of the view.
 *
 * The way this is accomplished is by using the EntityMimeTypeFilterModel for splitting the model into a "CollectionTree" and an "Item List"
 * as in the above example, and using a type-specific EntityTreeModel subclass to return the type-specific data, typically for only one type (for example, contacts or emails).
 *
 * The following protected virtual methods should be implemented in the subclass:
 * - int entityColumnCount( HeaderGroup headerGroup ) const;
 * -- Implement to return the number of columns for a HeaderGroup. If the HeaderGroup is CollectionTreeHeaders, return the number of columns to display for the
 *    Collection tree, and if it is ItemListHeaders, return the number of columns to display for the item. In the case of addressee, this could be for example,
 *    two (for given name and family name) or for emails it could be three (for subject, sender, date). This is a decision of the subclass implementor.
 * - QVariant entityHeaderData( int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup ) const;
 * -- Implement to return the data for each section for a HeaderGroup. For example, if the header group is CollectionTreeHeaders in a contacts model,
 *    the string "Address books" might be returned for column 0, whereas if the headerGroup is ItemListHeaders, the strings "Given Name", "Family Name",
 *    "Email Address" might be returned for the columns 0, 1, and 2.
 * - QVariant entityData( const Collection &collection, int column, int role = Qt::DisplayRole ) const;
 * -- Implement to return data for a particular Collection. Typically this will be the name of the collection or the EntityDisplayAttribute.
 * - QVariant entityData( const Item &item, int column, int role = Qt::DisplayRole ) const;
 * -- Implement to return the data for a particular item and column. In the case of email for example, this would be the actual subject, sender and date of the email.
 *
 * @note The entityData methods are just for convenience. the QAbstractItemMOdel::data method can be overridden if required.
 *
 * The application writer must then properly configure proxy models for the views, so that the correct data is shown in the correct view.
 * That is the purpose of these lines in the above example
 *
 * @code
 * collectionTree->setHeaderGroup( EntityTreeModel::CollectionTreeHeaders );
 * itemList->setHeaderGroup( EntityTreeModel::ItemListHeaders );
 * @endcode
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADI_EXPORT EntityTreeModel : public QAbstractItemModel
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
      ParentCollectionRole,                   ///< The parent collection of the entity
      ColumnCountRole,                        ///< @internal Used by proxies to determine the number of columns for a header group.
      LoadedPartsRole,                        ///< Parts available in the model for the item
      AvailablePartsRole,                     ///< Parts available in the Akonadi server for the item
      SessionRole,                            ///< @internal The Session used by this model
      CollectionRefRole,                      ///< @internal Used to increase the reference count on a Collection
      CollectionDerefRole,                    ///< @internal Used to decrease the reference count on a Collection
      PendingCutRole,                         ///< @internal Used to indicate items which are to be cut
      EntityUrlRole,                          ///< The akonadi:/ Url of the entity as a string. Item urls will contain the mimetype.
      UserRole = Qt::UserRole + 500,          ///< First role for user extensions.
      TerminalUserRole = 2000,                ///< Last role for user extensions. Don't use a role beyond this or headerData will break.
      EndRole = 65535
    };


    /**
     * Describes what header information the model shall return.
     */
    enum HeaderGroup {
      EntityTreeHeaders,      ///< Header information for a tree with collections and items
      CollectionTreeHeaders,  ///< Header information for a collection-only tree
      ItemListHeaders,        ///< Header information for a list of items
      UserHeaders = 10,       ///< Last header information for submodel extensions
      EndHeaderGroup = 32     ///< Last headergroup role. Don't use a role beyond this or headerData will break.
      // Note that we're splitting up available roles for the header data hack and int(EndRole / TerminalUserRole) == 32
    };

    /**
     * Creates a new entity tree model.
     *
     * @param monitor The ChangeRecorder whose entities should be represented in the model.
     * @param parent The parent object.
     */
    explicit EntityTreeModel( ChangeRecorder *monitor, QObject *parent = 0 );

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
     * Some Entities are hidden in the model, but exist for internal purposes, for example, custom object
     * directories in groupware resources.
     *
     * They are hidden by default, but can be shown by setting @p show to true.
     *
     * Most applications will not need to use this feature.
     */
    void setShowSystemEntities( bool show );

    /**
     * Returns @c true if internal system entities are shown, and @c false otherwise.
     */
    bool systemEntitiesShown() const;

    /**
     * Sets the item population @p strategy of the model.
     */
    void setItemPopulationStrategy( ItemPopulationStrategy strategy );

    /**
     * Returns the item population strategy of the model.
     */
    ItemPopulationStrategy itemPopulationStrategy() const;

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
     * Reimplemented to handle the AmazingCompletionRole.
     */
    virtual QModelIndexList match( const QModelIndex& start, int role, const QVariant& value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap ) ) const;

  protected:
    /**
     * Clears and resets the model. Always call this instead of the reset method in the superclass.
     * Using the reset method will not reliably clear or refill the model.
     */
    void clearAndReset();

    /**
     * Provided for convenience of subclasses.
     */
    virtual QVariant entityData( const Item &item, int column, int role = Qt::DisplayRole ) const;

    /**
     * Provided for convenience of subclasses.
     */
    virtual QVariant entityData( const Collection &collection, int column, int role = Qt::DisplayRole ) const;

    /**
     * Reimplement this to provide different header data. This is needed when using one model
     * with multiple proxies and views, and each should show different header data.
     */
    virtual QVariant entityHeaderData( int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup ) const;

    virtual int entityColumnCount( HeaderGroup headerGroup ) const;

    /**
     * Reimplement this in a subclass to return true if @p item matches @p value with @p flags in the AmazingCompletionRole.
     */
    virtual bool entityMatch( const Item &item, const QVariant &value, Qt::MatchFlags flags ) const;

    /**
     * Reimplement this in a subclass to return true if @p collection matches @p value with @p flags in the AmazingCompletionRole.
     */
    virtual bool entityMatch( const Collection &collection, const QVariant &value, Qt::MatchFlags flags ) const;

protected:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE( EntityTreeModel )
    EntityTreeModelPrivate * d_ptr;
    EntityTreeModel( ChangeRecorder *monitor, EntityTreeModelPrivate *d, QObject* parent = 0 );
    //@endcond

private:
  //@cond PRIVATE
    // Make these private, they shouldn't be called by applications
    virtual bool insertRows( int , int, const QModelIndex& = QModelIndex() );
    virtual bool insertColumns( int, int, const QModelIndex& = QModelIndex() );
    virtual bool removeColumns( int, int, const QModelIndex& = QModelIndex() );
    virtual bool removeRows( int, int, const QModelIndex & = QModelIndex() );

    Q_PRIVATE_SLOT( d_func(), void monitoredCollectionStatisticsChanged( Akonadi::Collection::Id,
                                                                         const Akonadi::CollectionStatistics& ) )

    Q_PRIVATE_SLOT( d_func(), void rootCollectionFetched(Akonadi::Collection::List) )
    Q_PRIVATE_SLOT( d_func(), void startFirstListJob() )
    // Q_PRIVATE_SLOT( d_func(), void slotModelReset() )

    // TODO: Can I merge these into one jobResult slot?
    Q_PRIVATE_SLOT( d_func(), void fetchJobDone( KJob *job ) )
    Q_PRIVATE_SLOT( d_func(), void pasteJobDone( KJob *job ) )
    Q_PRIVATE_SLOT( d_func(), void updateJobDone( KJob *job ) )

    Q_PRIVATE_SLOT( d_func(), void itemsFetched( Akonadi::Item::List ) )
    Q_PRIVATE_SLOT( d_func(), void collectionsFetched( Akonadi::Collection::List ) )
    Q_PRIVATE_SLOT( d_func(), void topLevelCollectionsFetched( Akonadi::Collection::List ) )
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
