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

#include "akonadicore_export.h"
#include "collection.h"
#include "collectionfetchscope.h"
#include "item.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QStringList>

namespace Akonadi {

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
 * Models provide an interface for viewing, updating, deleting and moving Items and Collections.
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
 * have a valid QModelIndex, use modelIndexForCollection.
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
 *   changeRecorder->setMimeTypeMonitored( KContacts::addresseeMimeType() );
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
 * changeRecorder->setMimeTypeMonitored( KContacts::addresseeMimeType() );
 * @endcode
 *
 * to a different mimetype. KContacts::addresseeMimeType() is an alias for "text/directory". If changed to KMime::Message::mimeType()
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
 * If items are to be fetched by the model, it is necessary to specify which parts of the items
 * are to be fetched, using the ItemFetchScope class. By default, only the basic metadata is
 * fetched. To fetch all item data, including all attributes:
 *
 * @code
 * changeRecorder->itemFetchScope().fetchFullPayload();
 * changeRecorder->itemFetchScope().fetchAllAttributes();
 * @endcode
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
 * <h3>Progress reporting</h3>
 *
 * The EntityTreeModel uses asynchronous Akonadi::Job instances to fill and update itself.
 * For example, a job is run to fetch the contents of collections (that is, list the items in it).
 * Additionally, individual Akonadi::Items can be fetched in different parts at different times.
 *
 * To indicate that such a job is underway, the EntityTreeModel makes the FetchState available. The
 * FetchState returned from a QModelIndex representing a Akonadi::Collection will be FetchingState if a
 * listing of the items in that collection is underway, otherwise the state is IdleState.
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADICORE_EXPORT EntityTreeModel : public QAbstractItemModel
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
        UnreadCountRole,                        ///< Returns the number of unread items in a collection. @since 4.5
        FetchStateRole,                         ///< Returns the FetchState of a particular item. @since 4.5
        CollectionSyncProgressRole,             ///< Returns the progress of synchronization in percent for a particular collection. @since 4.6. Deprecated since 4.14.3.
        IsPopulatedRole,                        ///< Returns whether a Collection has been populated, i.e. whether its items have been fetched. @since 4.10
        UserRole = Qt::UserRole + 500,          ///< First role for user extensions.
        TerminalUserRole = 2000,                ///< Last role for user extensions. Don't use a role beyond this or headerData will break.
        EndRole = 65535
    };

    /**
     * Describes the state of fetch jobs related to particular collections.
     *
     * @code
     *   QModelIndex collectionIndex = getIndex();
     *   if (collectionIndex.data(EntityTreeModel::FetchStateRole).toLongLong() == FetchingState) {
     *     // There is a fetch underway
     *   } else {
     *     // There is no fetch underway.
     *   }
     * @endcode
     *
     * @since 4.5
     */
    enum FetchState {
        IdleState,                              ///< There is no fetch of items in this collection in progress.
        FetchingState                           ///< There is a fetch of items in this collection in progress.
        // TODO: Change states for reporting of fetching payload parts of items.
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
    explicit EntityTreeModel(ChangeRecorder *monitor, QObject *parent = Q_NULLPTR);

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
     * They are hidden by default, but can be shown by setting @p show to true.
     * @param show enabled displaying of hidden entities if set as @c true
     * Most applications will not need to use this feature.
     */
    void setShowSystemEntities(bool show);

    /**
     * Returns @c true if internal system entities are shown, and @c false otherwise.
     */
    bool systemEntitiesShown() const;

    /**
     * Returns whether unsubscribed entities will be included in the listing.
     *
     * @since 4.5
     * @deprecated use listFilter instead
     */
    AKONADICORE_DEPRECATED bool includeUnsubscribed() const;

    /**
     * Sets whether unsubscribed entities will be included in the listing.
     * By default it's true
     * @param show enables displaying of unsubscribed entities if set as @c true
     * Note that it is possible to change the monitor's fetchscope directly,
     *  bypassing this method, which will lead to inconsistencies. Use
     *  this method for turning on/off listing of subscribed folders.
     *
     * @since 4.5
     * @deprecated use setListFilter instead
     */
    AKONADICORE_DEPRECATED void setIncludeUnsubscribed(bool show);

    /**
     * Returns the currently used listfilter.
     *
     * @since 4.14
     */
    Akonadi::CollectionFetchScope::ListFilter listFilter() const;

    /**
     * Sets the currently used listfilter.
     *
     * @since 4.14
     */
    void setListFilter(Akonadi::CollectionFetchScope::ListFilter filter);

    /**
     * Monitors the specified collections and resets the model.
     *
     * @since 4.14
     */
    void setCollectionsMonitored(const Akonadi::Collection::List &collections);

    /**
     * Adds or removes a specific collection from the monitored set without resetting the model.
     * Only call this if you're monitoring specific collections (not mimetype/resources/items).
     *
     * @since 4.14
     * @see setCollectionsMonitored()
     */
    void setCollectionMonitored(const Akonadi::Collection &col, bool monitored = true);

    /**
     * References a collection and starts to monitor it.
     *
     * Use this to temporarily include a collection that is not enabled.
     *
     * @since 4.14
     */
    void setCollectionReferenced(const Akonadi::Collection &col, bool referenced = true);

    /**
     * Sets the item population @p strategy of the model.
     */
    void setItemPopulationStrategy(ItemPopulationStrategy strategy);

    /**
     * Returns the item population strategy of the model.
     */
    ItemPopulationStrategy itemPopulationStrategy() const;

    /**
     * Sets whether the root collection shall be provided by the model.
     * @param include enables root collection if set as @c true
     * @see setRootCollectionDisplayName()
     */
    void setIncludeRootCollection(bool include);

    /**
     * Returns whether the root collection is provided by the model.
     */
    bool includeRootCollection() const;

    /**
     * Sets the display @p name of the root collection of the model.
     * The default display name is "[*]".
     * @param name the name to display for the root collection
     * @note The display name for the root collection is only used if
     *       the root collection has been included with setIncludeRootCollection().
     */
    void setRootCollectionDisplayName(const QString &name);

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
        FetchCollectionsRecursive,        ///< Fetches collections in the root collection recursively. This is the default.
        InvisibleCollectionFetch          ///< Fetches collections, but does not put them in the model. This can be used to create a list of items in all collections. The ParentCollectionRole can still be used to retrieve the parent collection of an Item. @since 4.5
    };

    /**
     * Sets the collection fetch @p strategy of the model.
     */
    void setCollectionFetchStrategy(CollectionFetchStrategy strategy);

    /**
     * Returns the collection fetch strategy of the model.
     */
    CollectionFetchStrategy collectionFetchStrategy() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QStringList mimeTypes() const Q_DECL_OVERRIDE;

    Qt::DropActions supportedDropActions() const Q_DECL_OVERRIDE;
    QMimeData *mimeData(const QModelIndexList &indexes) const Q_DECL_OVERRIDE;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

    // TODO: Review the implementations of these. I think they could be better.
    bool canFetchMore(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    void fetchMore(const QModelIndex &parent) Q_DECL_OVERRIDE;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    /**
     * Returns whether the collection tree has been fetched at initialisation.
     *
     * @see collectionTreeFetched
     * @since 4.10
     */
    bool isCollectionTreeFetched() const;

    /**
     * Returns whether the collection has been populated.
     *
     * @see collectionPopulated
     * @since 4.12
     */
    bool isCollectionPopulated(Akonadi::Collection::Id) const;

    /**
     * Returns whether the model is fully populated.
     *
     * Returns true once the collection tree has been fetched and all collections have been populated.
     *
     * @see isCollectionPopulated
     * @see isCollectionTreeFetched
     * @since 4.14
     */
    bool isFullyPopulated() const;

    /**
     * Reimplemented to handle the AmazingCompletionRole.
     */
    QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const Q_DECL_OVERRIDE;

    /**
     * Returns a QModelIndex in @p model which points to @p collection.
     * This method can be used through proxy models if @p model is a proxy model.
     * @code
     * EntityTreeModel *model = getEntityTreeModel();
     * QSortFilterProxyModel *proxy1 = new QSortFilterProxyModel;
     * proxy1->setSourceModel(model);
     * QSortFilterProxyModel *proxy2 = new QSortFilterProxyModel;
     * proxy2->setSourceModel(proxy1);
     *
     * ...
     *
     * QModelIndex idx = EntityTreeModel::modelIndexForCollection(proxy2, Collection(colId));
     * if (!idx.isValid())
     *   // Collection with id colId is not in the proxy2.
     *   // Maybe it is filtered out if proxy 2 is only showing items? Make sure you use the correct proxy.
     *   return;
     *
     * Collection collection = idx.data( EntityTreeModel::CollectionRole ).value<Collection>();
     * // collection has the id colId, and all other attributes already fetched by the model such as name, remoteId, Akonadi::Attributes etc.
     *
     * @endcode
     *
     * This can be useful for example if an id is stored in a config file and needs to be used in the application.
     *
     * Note however, that to restore view state such as scrolling, selection and expansion of items in trees, the ETMViewStateSaver can be used for convenience.
     *
     * @see modelIndexesForItem
     * @since 4.5
     */
    static QModelIndex modelIndexForCollection(const QAbstractItemModel *model, const Collection &collection);

    /**
     * Returns a QModelIndex in @p model which points to @p item.
     * This method can be used through proxy models if @p model is a proxy model.
     * @param model the model to query for the item
     * @param item the item to look for
     * @see modelIndexForCollection
     * @since 4.5
     */
    static QModelIndexList modelIndexesForItem(const QAbstractItemModel *model, const Item &item);

Q_SIGNALS:
    /**
     * Signal emitted when the collection tree has been fetched for the first time.
     * @param collections  list of collections which have been fetched
     *
     * @see isCollectionTreeFetched, collectionPopulated
     * @since 4.10
     */
    void collectionTreeFetched(const Akonadi::Collection::List &collections);

    /**
     * Signal emitted when a collection has been populated, i.e. its items have been fetched.
     * @param collectionId  id of the collection which has been populated
     *
     * @see collectionTreeFetched
     * @since 4.10
     */
    void collectionPopulated(Akonadi::Collection::Id collectionId);
    /**
     * Emitted once a collection has been fetched for the very first time.
     * This is like a dataChanged(), but specific to the initial loading, in order to update
     * the GUI (window caption, state of actions).
     * Usually, the GUI uses Akonadi::Monitor to be notified of further changes to the collections.
     * @param collectionId the identifier of the fetched collection
     * @since 4.9.3
     */
    void collectionFetched(int collectionId);

protected:
    /**
     * Clears and resets the model. Always call this instead of the reset method in the superclass.
     * Using the reset method will not reliably clear or refill the model.
     */
    void clearAndReset();

    /**
     * Provided for convenience of subclasses.
     */
    virtual QVariant entityData(const Item &item, int column, int role = Qt::DisplayRole) const;

    /**
     * Provided for convenience of subclasses.
     */
    virtual QVariant entityData(const Collection &collection, int column, int role = Qt::DisplayRole) const;

    /**
     * Reimplement this to provide different header data. This is needed when using one model
     * with multiple proxies and views, and each should show different header data.
     */
    virtual QVariant entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const;

    virtual int entityColumnCount(HeaderGroup headerGroup) const;

    /**
     * Reimplement this in a subclass to return true if @p item matches @p value with @p flags in the AmazingCompletionRole.
     */
    virtual bool entityMatch(const Item &item, const QVariant &value, Qt::MatchFlags flags) const;

    /**
     * Reimplement this in a subclass to return true if @p collection matches @p value with @p flags in the AmazingCompletionRole.
     */
    virtual bool entityMatch(const Collection &collection, const QVariant &value, Qt::MatchFlags flags) const;

protected:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(EntityTreeModel)
    EntityTreeModelPrivate *d_ptr;
    EntityTreeModel(ChangeRecorder *monitor, EntityTreeModelPrivate *d, QObject *parent = Q_NULLPTR);
    //@endcond

private:
    //@cond PRIVATE
    // Make these private, they shouldn't be called by applications
    bool insertRows(int row, int count, const QModelIndex &index = QModelIndex()) Q_DECL_OVERRIDE;
    bool insertColumns(int column, int count, const QModelIndex &index = QModelIndex()) Q_DECL_OVERRIDE;
    bool removeColumns(int column, int count, const QModelIndex &index = QModelIndex()) Q_DECL_OVERRIDE;
    bool removeRows(int row, int count, const QModelIndex &index = QModelIndex()) Q_DECL_OVERRIDE;

    Q_PRIVATE_SLOT(d_func(), void monitoredCollectionStatisticsChanged(Akonadi::Collection::Id,
                                                                       const Akonadi::CollectionStatistics &))

    Q_PRIVATE_SLOT(d_func(), void startFirstListJob())
    Q_PRIVATE_SLOT(d_func(), void serverStarted())

    Q_PRIVATE_SLOT(d_func(), void itemFetchJobDone(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void collectionFetchJobDone(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void rootFetchJobDone(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void pasteJobDone(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void updateJobDone(KJob *job))

    Q_PRIVATE_SLOT(d_func(), void itemsFetched(Akonadi::Item::List))
    Q_PRIVATE_SLOT(d_func(), void collectionsFetched(Akonadi::Collection::List))
    Q_PRIVATE_SLOT(d_func(), void collectionListFetched(Akonadi::Collection::List))
    Q_PRIVATE_SLOT(d_func(), void topLevelCollectionsFetched(Akonadi::Collection::List))
    Q_PRIVATE_SLOT(d_func(), void ancestorsFetched(Akonadi::Collection::List))

    Q_PRIVATE_SLOT(d_func(), void monitoredMimeTypeChanged(const QString &, bool))
    Q_PRIVATE_SLOT(d_func(), void monitoredCollectionsChanged(const Akonadi::Collection &, bool))
    Q_PRIVATE_SLOT(d_func(), void monitoredItemsChanged(const Akonadi::Item &, bool))
    Q_PRIVATE_SLOT(d_func(), void monitoredResourcesChanged(const QByteArray &, bool))

    Q_PRIVATE_SLOT(d_func(), void monitoredCollectionAdded(const Akonadi::Collection &, const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void monitoredCollectionRemoved(const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void monitoredCollectionChanged(const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void monitoredCollectionMoved(const Akonadi::Collection &, const Akonadi::Collection &,
                                                           const Akonadi::Collection &))

    Q_PRIVATE_SLOT(d_func(), void monitoredItemAdded(const Akonadi::Item &, const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void monitoredItemRemoved(const Akonadi::Item &))
    Q_PRIVATE_SLOT(d_func(), void monitoredItemChanged(const Akonadi::Item &, const QSet<QByteArray> &))
    Q_PRIVATE_SLOT(d_func(), void monitoredItemMoved(const Akonadi::Item &, const Akonadi::Collection &,
                                                     const Akonadi::Collection &))

    Q_PRIVATE_SLOT(d_func(), void monitoredItemLinked(const Akonadi::Item &, const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void monitoredItemUnlinked(const Akonadi::Item &, const Akonadi::Collection &))
    Q_PRIVATE_SLOT(d_func(), void changeFetchState(const Akonadi::Collection &))

    Q_PRIVATE_SLOT(d_func(), void agentInstanceRemoved(Akonadi::AgentInstance))
    Q_PRIVATE_SLOT(d_func(), void monitoredItemsRetrieved(KJob *job))
    //@endcond
};

} // namespace

#endif
