# Akonadi client libraries # {#client_libraries}

[TOC]

Akonadi client libraries consist of three libraries that provide tools to access
the Akonadi PIM data server: AkonadiCore, AkonadiWidgets and AkonadiAgentBase.
All processes accessing Akonadi, including those which communicate with a remote
server [agents](@ref agents), are considered clients.

<!--
Additional information about Akonadi:

- <a href="https://api.kde.org/kdesupport-api/akonadi-apidocs/">Akonadi Server documentation</a>
- \ref akonadi_history
- <a href="https://community.kde.org/KDE_PIM/Akonadi">Website</a>
- <a href="https://techbase.kde.org/KDE_PIM/Akonadi">Wiki</a>

Tools for developers:

- <a href="https://bugs.kde.org/buglist.cgi?query_format=advanced&product=Akonadi&component=libakonadi&bug_status=REPORTED&bug_status=CONFIRMED&bug_status=ASSIGNED&bug_status=REOPENED">Bugtracker</a>
//-->

# Akonadi Objects # {#objects}

Akonadi works on two basic object types: collections and items.

Collections are comparable to folders in a file system and are represented by
the class Akonadi::Collection. Every collection has an associated cache policy
represented by the class Akonadi::CachePolicy which defines what part of its
content is cached for how long. All available ways to work with collections are
listed in the "[Collections](#collections)" section.

Akonadi items are comparable to files in a file system and are represented by
the class Akonadi::Item. Each item represents a single PIM object such as a mail
or a contact. The actual object it represents is its so-called payload. All
available ways to work with items are listed in the "[Items](#items)"
section.

Both items and collections are identified by a persistent unique identifier.
Also, they can contain arbitrary attributes (derived from Akonadi::Attribute) to
attach general or application specific meta data to them.

# Collection retrieval and manipulation # {#collections}

A collection is represented by the Akonadi::Collection class.

Classes to retrieve information about collections:

* Akonadi::CollectionFetchJob
* Akonadi::CollectionStatisticsJob

Classes to manipulate collections:

* Akonadi::CollectionCreateJob
* Akonadi::CollectionCopyJob
* Akonadi::CollectionModifyJob
* Akonadi::CollectionDeleteJob

There is also Akonadi::CollectionModel, which is a self-updating model class which can
be used in combination with Akonadi::CollectionView. Akonadi::CollectionFilterProxyModel
can be used to limit a displayed collection tree to collections supporting a certain
type of PIM items. Akonadi::CollectionPropertiesDialog provides an extensible properties
dialog for collections. Often needed QAction for collection operations are provided by
Akonadi::StandardActionManager.

# PIM item retrieval and manipulation # {#items}

PIM items are represented by classes derived from Akonadi::Item.
Items can be retrieved using Akonadi::ItemFetchJob.

The following classes are provided to manipulate PIM items:

* Akonadi::ItemCreateJob
* Akonadi::ItemCopyJob
* Akonadi::ItemModifyJob
* Akonadi::ItemDeleteJob

Akonadi::ItemModel provides a self-updating model class which can be used to display the content
of a collection. Akonadi::ItemView is the base class for a corresponding view. Often needed QAction
for item operations are provided by Akonadi::StandardActionManager.

# Low-level access to the Akonadi server # {#jobs}

Accessing the Akonadi server is done using job classes derived from Akonadi::Job. The
communication channel with the server is provided by Akonadi::Session.

To use server-side transactions, the following jobs are provided:

* Akonadi::TransactionBeginJob
* Akonadi::TransactionCommitJob
* Akonadi::TransactionRollbackJob

There also is Akonadi::TransactionSequence which can be used to automatically group
a set of jobs into a single transaction.


# Change notifications (Monitor) # {#monitor}

The Akonadi::Monitor class allows you to monitor specific resources,
collections and PIM items for changes. Akonadi::ChangeRecorder augments this
by providing a way to record and replay change notifications.

# PIM item serializer # {#serializer}

The class Akonadi::ItemSerializer is responsible for converting between the stored (binary) representation
of a PIM item and the objects used to handle these items provided by the corresponding libraries (kabc, kcal, etc.).

Serializer plugins allow you to add support for new kinds of PIM items to Akonadi.
Akonadi::ItemSerializerPlugin can be used as a base class for such a plugin.

# Agents and Resources # {#resource}

Agents are independent processes that watch the Akonadi store for changes and react to them if necessary.
Example: The Akonadi Indexing Agent is an agent that watches Akonadi for new emails, calendar events, etc.,
retrieves the new items and indexes them into a special database.

The class Akonadi::AgentBase is the common base class for all agents. It provides commonly needed
functionality such as change monitoring and recording.

Resources are a special kind of agents. They are used as the actual backend for whatever data the resource represents.
In this situation the akonadi server acts more like a proxy service. It caches data on behalf of its clients
(client here being the Resource), not permanently storing it. The Akonadi server forwards item retrieval requests to the
corresponding resource, if the item is not in the cache.
Example: The imap resource is responsible for storing and fetching emails from an imap server.

Akonadi::ResourceBase is the base class for them. It provides the
necessary interfaces to the server as well as many convenience functions to make implementing
a new resource as easy as possible. Note that a collection contains items belonging to a single
resource, although there are plans in the future for 'virtual' collections which will contain
the results of a search query spanning multiple resources.

A resource can support multiple mimetypes. There are two places where a resource can specify
mimetypes: in its desktop files, and in the content mimetypes field of
collections created by it. The ones in the desktop file are used for
filtering agent types, e.g. in the resource creation dialogs. The collection
content mimetypes specify what you can actually put into a collection, which
is not necessarily the same (e.g. the Kolab resource supports contacts and events, but not
in the same folder).


# Integration in your Application # {#integration}

Akonadi::Control provides ways to ensure that the Akonadi server is running, to monitor its availability
and provide help on server-side errors. A more low-level interface to the Akonadi server is provided
by Akonadi::ServerManager.

A set of standard actions is provided by Akonadi::StandardActionManager. These provide consistent
look and feel across applications.


This library provides classes for KDE applications to communicate with the Akonadi server. The most high-level interface to Akonadi is the Models and Views provided in this library. Ready to use models are provided for use with views to interact with a tree of collections, a list of items in a collection, or a combined tree of Collections and items.

## Collections and Items ## {#collections_and_items}

In the Akonadi concept, Items are individual objects of PIM data, e.g. emails, contacts, events, notes etc. The data in an item is stored in a typed payload. For example, if an Akonadi Item holds a contact, the contact is available as a KABC::Addressee:

~~~~~~~~~~~~~{.cpp}
if (item.hasPayload<KABC::Addressee>()) {
    KABC::Addressee addr = item.payload<KABC::Addressee>();
    // use addr in some way...
}
~~~~~~~~~~~~~

Additionally, an Item must have a mimetype which corresponds to the type of payload it holds.

Collections are simply containers of Items. A Collection has a name and a list of mimetypes that it may contain. A collection may for example contain events if it can contain the mimetype 'text/calendar'. A Collection itself (as opposed to its contents) has a mimetype, which is the same for all Collections. A Collection which can itself contain Collections must be able to contain the Collection mimetype.

~~~~~~~~~~~~~{.cpp}
Collection col;
// This collection can contain events and nested collections.
col.setContentMimetypes({ Akonadi::Collection::mimeType(),
                          QStringLiteral("text/calendar") });
~~~~~~~~~~~~~

This system makes it simple to create PIM applications. For example, to create an application for viewing and editing events, you simply need to tell %Akonadi to retrieve all items matching the mimetype 'text/calendar'.

## Convenience Mimetype Accessors ## {#convenience_mimetype_accessors}

In order to avoid typos, improve readability, and to encapsulate the correct mimetypes for particular pim items, many of the standard classes have an accessor for the kind of mimetype the can handle. For example, you can use KMime::Message::mimeType() for emails, KABC::Addressee::mimeType() for contacts etc. It makes sense to define a similar static function in your own types.

~~~~~~~~~~~~~{.cpp}
col.setContentMimetypes({ Akonadi::Collection::mimeType(),
                          KABC::Addressee::mimeType(),
                          KMime::Message::mimeType() });
~~~~~~~~~~~~~

## Models and Views ## {#models_and_views}
Akonadi models and views are a high level way to interact with the Akonadi server. Most applications will use these classes. See the EntityTreeModel documentation for more information.

Models provide an interface for viewing, deleting and moving Items and Collections. New Items can also be created by dropping data of the appropriate type on a model. Additionally, the models are updated automatically if another application changes the data or inserts or deletes items etc.

Akonadi provides several models for particular uses, e.g. the MailModel is used for emails and the ContactsModel is used for showing contacts. Additional specific models can be implemented using EntityTreeModel as a base class.

A typical use of these would be to create a model and use proxy models to make the view show different parts of the model. For example, show a collection tree in on one side and show items in a selected collection in another view.

~~~~~~~~~~~~~{.cpp}
mailModel = new MailModel(session, monitor, this);

collectionTree = new Akonadi::EntityMimeTypeFilterModel(this);
collectionTree->setSourceModel(mailModel);
// Filter out everything that is not a collection.
collectionTree->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());
collectionTree->setHeaderSet(Akonadi::EntityTreeModel::CollectionTreeHeaders);

collectionView = new Akonadi::EntityTreeView(this);
collectionView->setModel(collectionTree);

itemList = new Akonadi::EntityMimeTypeFilterModel(this);
itemList->setSourceModel(mailModel);
// Filter out collections
itemList->addMimeTypeExclusionFilter(Akonadi::Collection::mimeType());
itemList->setHeaderSet(Akonadi::EntityTreeModel::ItemListHeaders);

itemView = new Akonadi::EntityTreeView(this);
itemView->setModel(itemList);
~~~~~~~~~~~~~

![An email application using MailModel](/docs/images/mailmodelapp.png "An email application using MailModel")

The content of the model is determined by the configuration of the Monitor passed into it. The examples below show a use of the EntityTreeModel and some proxy models for a simple heirarchical note collection. As the model is generic, the configuration and proxy models will also work with any other mimetype.

~~~~~~~~~~~~~{.cpp}
// Configure what should be shown in the model:
Monitor *monitor = new Akonadi::Monitor(this);
monitor->fetchCollection(true);
monitor->setItemFetchScope(scope);
monitor->setCollectionMonitored(Akonadi::Collection::root());
monitor->setMimeTypeMonitored(MyEntity::mimeType());

Akonadi::Session *session = new Akonadi::Session(QByteArray("MyEmailApp-") + QByteArray::number(qrand()), this);
monitor->setSession(session);

Akonadi::EntityTreeModel *entityTree = new Akonadi::EntityTreeModel(monitor, this);
~~~~~~~~~~~~~

![A plain EntityTreeModel in a view](/docs/images/entitytreemodel.png "A plain EntityTreeModel in a view")

The EntityTreeModel can be further configured for certain behaviours such as fetching of collections and items.

To create a model of only a collection tree and no items, set that in the model. This is just like CollectionModel:

~~~~~~~~~~~~~{.cpp}
entityTree->setItemPopulationStrategy(Akonadi::EntityTreeModel::NoItemPopulation);
~~~~~~~~~~~~~

![A plain EntityTreeModel which does not fetch items.](/docs/images/entitytreemodel-collections.png "A plain EntityTreeModel which does not fetch items.")

Or, create a model of only items and not child collections. This is just like ItemModel:

~~~~~~~~~~~~~{.cpp}
entityTree->setRootCollection(myCollection);
entityTree->setCollectionFetchStrategy(Akonadi::EntityTreeModel::FetchNoCollections);
~~~~~~~~~~~~~

Or, to create a model which includes items and first level collections:

~~~~~~~~~~~~~{.cpp}
entityTree->setCollectionFetchStrategy(Akonadi::EntityTreeModel::FetchFirstLevelCollections);
~~~~~~~~~~~~~

The items in the model can also be inserted lazily for performance reasons. The Collection tree is always built immediately.

Additionally, a KDescendantsProxyModel may be used to alter how the items in the tree are presented.

~~~~~~~~~~~~~{.cpp}
// ... Create an entityTreeModel
KDescendantsProxyModel *descProxy = new KDescendantsProxyModel(this);
descProxy->setSourceModel(entityTree);
view->setModel(descProxy);
~~~~~~~~~~~~~

![A KDescendantsProxyModel wrapping an EntityTreeModel](/docs/images/descendantentitiesproxymodel.png "A KDescendantsProxyModel wrapping an EntityTreeModel")

KDescendantsProxyModel can also display ancestors of each Entity in the list.

~~~~~~~~~~~~~{.cpp}
// ... Create an entityTreeModel
KDescendantsProxyModel *descProxy = new KDescendantsProxyModel(this);
descProxy->setSourceModel(entityTree);

// #### This is new
descProxy->setDisplayAncestorData(true, QLatin1String(" / "));

view->setModel(descProxy);
~~~~~~~~~~~~~

![A KDescendantsProxyModel with ancestor names.](/docs/images/descendantentitiesproxymodel-withansecnames.png "A KDescendantsProxyModel with ancestor names.")

This proxy can be combined with a filter to for example remove collections.

~~~~~~~~~~~~~{.cpp}
// ... Create an entityTreeModel
KDescendantsProxyModel *descProxy = new KDescendantsProxyModel(this);
descProxy->setSourceModel(entityTree);

// #### This is new.
Akonadi::EntityMimeTypeFilterModel *filterModel = new Akonadi::EntityMimeTypeFilterModel(this);
filterModel->setSourceModel(descProxy);
filterModel->setExclusionFilter({ Akonadi::Collection::mimeType() });

view->setModel(filterModel);
~~~~~~~~~~~~~

![An EntityMimeTypeFilterModel wrapping a KDescendantsProxyModel wrapping an EntityTreeModel](/docs/images/descendantentitiesproxymodel-colfilter.png "An EntityMimeTypeFilterModel wrapping a KDescendantsProxyModel wrapping an EntityTreeModel")

It is also possible to show the root item as part of the selectable model:

~~~~~~~~~~~~~{.cpp}
entityTree->setIncludeRootCollection(true);
~~~~~~~~~~~~~

![An EntityTreeModel showing Collection::root](/docs/images/entitytreemodel-showroot.png "An EntityTreeModel showing Collection::root")

By default the displayed name of the root collection is '[*]', because it doesn't require i18n, and is generic. It can be changed too.

~~~~~~~~~~~~~{.cpp}
entityTree->setIncludeRootCollection(true);
entityTree->setRootCollectionDisplayName(i18nc("Name of top level for all collections in the application", "[All]"))
~~~~~~~~~~~~~

![An EntityTreeModel showing Collection::root with an application specific name.](/docs/images/entitytreemodel-showrootwithname.png "An EntityTreeModel showing Collection::root with an application specific name.")

These can of course be combined to create an application which uses one EntityTreeModel along with several proxies and views.

~~~~~~~~~~~~~{.cpp}
// ... create an EntityTreeModel.
Akonadi::EntityMimeTypeFilterModel *collectionTree = new Akonadi::EntityMimeTypeFilterModel(this);
collectionTree->setSourceModel(entityTree);
// Filter to include collections only:
collectionTree->setInclusionFilter({ Akonadi:: Collection::mimeType() });
Akonadi::EntityTreeView *treeView = new Akonadi::EntityTreeView(this);
treeView->setModel(collectionTree);

Akonadi::EntityMimeTypeFilterModel *itemList = new Akonadi::EntityMimeTypeFilterModel(this);
itemList->setSourceModel(entityTree);
// Filter *out* collections
itemList->setExclusionFilter({ Akonadi::Collection::mimeType() });
Akonadi::EntityTreeView *listView = new Akonadi::EntityTreeView(this);
listView->setModel(itemList);
~~~~~~~~~~~~~

![A single EntityTreeModel with several views and proxies.](/docs/images/treeandlistapp.png "A single EntityTreeModel with several views and proxies.")

Or to also show items of child collections in the list:

~~~~~~~~~~~~~{.cpp}
// ... Create an entityTreeModel
collectionTree = new Akonadi::EntityMimeTypeFilterModel(this);
collectionTree->setSourceModel(entityTree);

// Include only collections in this proxy model.
collectionTree->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());

treeview->setModel(collectionTree);

descendedList = new KDescendantsProxyModel(this);
descendedList->setSourceModel(entityTree);

itemList = new Akonadi::EntityMimeTypeFilterModel(this);
itemList->setSourceModel(descendedList);

// Exclude collections from the list view.
itemList->addMimeTypeExclusionFilter(Akonadi::Collection::mimeType());

listView = new EntityTreeView(this);
listView->setModel(itemList);
~~~~~~~~~~~~~

![Showing descendants of all Collections in the list](/docs/images/treeandlistappwithdesclist.png "Showing descendants of all Collections in the list")

Note that it is important in this case to use the DescendantEntitesProxyModel before the EntityMimeTypeFilterModel. Otherwise, by filtering out the collections first, you would also be filtering out their child items.

A SelectionProxyModel can be used to simplify managing selection in one view through multiple proxy models to a representation in another view. The selectionModel of the initial view is used to create a proxied model which includes only the selected indexes and their children.


~~~~~~~~~~~~~{.cpp}
// ... Create an entityTreeModel
collectionTree = new Akonadi::EntityMimeTypeFilterModel(this);
collectionTree->setSourceModel(entityTree);

// Include only collections in this proxy model.
collectionTree->addMimeTypeInclusionFilter(Akonadi::Collection::mimeType());

treeview->setModel(collectionTree);

// SelectionProxyModel can handle complex selections:
treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

SelectionProxyModel *selProxy = new SelectionProxyModel(treeview->selectionModel(), this);
selProxy->setSourceModel(entityTree);

Akonadi::EntityTreeView *selView = new Akonadi::EntityTreeView(splitter);
selView->setModel(selProxy);
~~~~~~~~~~~~~

![A Selection in one view creating a model for use with another view.](/docs/images/selectionproxymodelsimpleselection.png "A Selection in one view creating a model for use with another view.")

The SelectionProxyModel can handle complex selections.

![Non-contiguous selection creating a new simple model in a second view](/docs/images//selectionproxymodelmultipleselection.png "Non-contiguous selection creating a new simple model in a second view.")

If an index and one or more of its descendants are selected, only the top-most selected index (including all of its descendants) are included in the proxy model. (Though this is configurable. See below)

![Selecting an item and its descendant](/docs/images/selectionproxymodelmultipleselection-withdescendant.png "Selecting an item and its descendant.")

SelectionProxyModel allows configuration using the methods setStartWithChildTrees, setOmitDescendants, setIncludeAllSelected. See testapp/proxymodeltestapp to try out the 5 valid configurations.

Obviously, the SelectionProxyModel may be used in a view, or further processed with other proxy models. See the example_contacts application for example which uses a further KDescendantsProxyModel and EntityMimeTypeFilterModel on top of a SelectionProxyModel.

The SelectionProxyModel orders its items in the same top-to-bottom order as they appear in the source model. Note that this order may be different to the order in the selection model if there is a QSortFilterProxyModel between the selection and the source model.

![Ordered items in the SelectionProxyModel](/docs/images/selectionproxymodel-ordered.png "Ordered items in the SelectionProxyModel")

Details on the actual implementation of lazy population are described on [this page](@ref internals).

# Jobs and Monitors # {#jobs_and_monitors}

The lower level way to interact with Akonadi is to use Jobs and Monitors (This is what models use internally). Jobs are used to make changes to akonadi, and in some cases (e.g., a fetch job) emit a signal with data resulting from the job. A Monitor reports changes made to the data stored in Akonadi (e.g., creating, updating, deleting or moving an item or collection ) via signals.

Typically, an application will configure a monitor to report changes to a particular Collection, mimetype or resource, and then connect to the signals it emits.

Most applications will use some of the low level api for actions unrelated to a model-tree view, such as creating new items and collections.

# Tricky details # {#tricky_details}

## Change Conflicts ## {#change_conflicts}
It is possible that while an application is editing an item, that item gets updated in akonadi. Akonadi will notify the application that that item has changed via a Monitor signal. It is the responsibility of the application to handle the conflict by for example offering the user a dialog to resolve it. Alternatively, the application could ignore the dataChanged signal for that item, and will get another chance to resolve the conflict when trying to save the result back to akonadi. In that case, the ItemModifyJob will fail and report that the revision number of the item on the server differs from its revision number as reported by the job. Again, it is up to the application to handle this case.

This is something that every application using akonadi will have to handle.

## Using Item::Id or Collection::Id as an identifier ## {#using_id_as_an_identifier}

Items and Collections have a id() member which is a unique identifier used by akonadi. It can be useful to use the id() as an identifier when storing Collections or Items.

However, as an item and a collection can have the same id(), if you need to store both Collections and Items together by a simple identifier, conflicts can occur.

~~~~~~~~~~~~~{.cpp}
QString getRemoteIdById(Item::Id id)
{
    // Note:
    // m_items is QHash<Item::Id, Item>
    // m_collections is QHash<Collection::Id, Collection>
    if (m_items.contains(id)) {
        // Oops, we could accidentally match a collection here.
        return m_items.value(id).remoteId();
    } else if (m_collections.contains(id)) {
        return m_collections.value(id).remoteId();
    }
    return QString();
}
~~~~~~~~~~~~~

In this case, it makes more sense to use a normal qint64 as the internal identifier, and use the sign bit to determine if the identifier refers to a Collection or an Item. This is done in the implementation of EntityTreeModel to tell Collections and Items apart.

~~~~~~~~~~~~~{.cpp}
qstring getremoteidbyinternalidentifier(qint64 internalidentifier)
{
    // note:
    // m_items is qhash<item::id, item>
    // m_collections is qhash<collection::id, collection>

    // if the id is negative, it refers to an item
    // otherwise it refers to a collection.

    if (internalidentifier < 0) {
        // reverse the sign of the id before using it.
        return m_items.value(-internalidentifier).remoteid();
    } else {
        return m_collections.value(internalidentifier).remoteid();
    }
}
~~~~~~~~~~~~~


### Unordered Lists ### {#unordered_lists}
Collection and Item both provide a ::List to represent groups of objects. However the objects in the list are usually not ordered in any particular way, even though the API provides methods to work with an ordered list. It makes more sense to think of it as a Set instead of a list in most cases.

For example, when using an ItemFetchJob to fetch the items in a collection, the items could be in any order when returned from the job. The order that a Monitor emits notices of changes is also indeterminate. By using a Transaction however, it is sometimes possible to retrieve objects in order. Additionally, using s constructor overload in the CollectionFetchJob it is possible to retrieve collections in a particular order.

~~~~~~~~~~~~~{.cpp}
Collection::List getCollections(const QList<Collection::Id> &idsToGet)
{
    Collection::List getList;
    for (Collection::Id id :  idsToGet) {
        getList << Collection(id);
    }
    CollectionFetchJob *job = CollectionFetchJob(getList);
    if (job->exec()) {
        // job->collections() is in the same order as the ids in idsToGet.
    }
}
~~~~~~~~~~~~~

# Resources # {#resources}
The KDEPIM module includes resources for handling many types of PIM data, such as imap email, vcard files and vcard directories, ical event files etc. These cover many of the sources for your PIM data, but in the case that you need to use data from another source (for example a website providing a contacts storage service and an api), you simply have to write a new resource.

https://techbase.kde.org/Development/Tutorials/Akonadi/Resources

# Serializers # {#serializers}
Serializers provide the functionality of converting raw data, for example from a file, to a strongly typed object of PIM data. For example, the addressee serializer reads data from a file and creates a KABC::Addressee object.

New serializers can also easily be written if the data you are dealing with is not one of the standard PIM data types.

# Implementation details # {#implementation_details}

## Updating Akonadi Models ## {#updating_models}

NOTE: The details here are only relevant if you are writing a new view using EntityTreeModel, or writing a new model.

Because communication with Akonadi happens asynchronously, and the models only hold a cached copy of the data on the akonadi server, some typical behaviours of models are not followed by Akonadi models.

For example, when setting data on a model via a view, most models syncronously update their internal store and notify akonadi to update its view of the data by returning <tt>true</tt>.

<!--
TODO: Render this manually
@dot
digraph utmg {
    rankdir = LR;
    { node [label="",style=invis, height=0, width=0 ];
      V_Set_Data; V_Result; V_Data_Changed;
      M_Set_Data; M_Result;
    }
    { node [shape=box, fillcolor=lightyellow, style=filled,fontsize=26];
      View [label=":View"]; Model [label=":Model"];
    }
    { node [style=invis];
      View_EOL; Model_EOL;
    }
    {
      V_Set_Data -> M_Set_Data [label="Set Data"];
      M_Result -> V_Result [label="Success",arrowhead="vee", style="dashed"];
      V_Result -> V_Data_Changed [label="Update View \n[ Success = True ]"];
    }

    // Dashed Vertical lines for object lifetimes.
    edge [style=dashed, arrowhead=none];
    { rank = same; View -> V_Set_Data -> V_Result -> V_Data_Changed -> View_EOL; }
    { rank = same; Model -> M_Set_Data -> M_Result -> Model_EOL; }

    // Make sure top nodes are in a straight line.
    { View -> Model [style=invis]; }
    // And the bottom nodes.
    { View_EOL -> Model_EOL [style=invis]; }
}
@enddot
//-->

Akonadi models only cache data from the Akonadi server. To update data on an Akonadi::Entity stored in a model, the model makes a request to the Akonadi server to update the model data. At that point the data cached internally in the model is not updated, so <tt>false</tt> is always returned from setData. If the request to update data on the Akonadi server is successful, an Akonadi::Monitor notifies the model that the data on that item has changed. The model then updates its internal data store and notifies the view that the data has changed. The details of how the Monitor communicates with akonadi are omitted for clarity.

<!--
TODO: Render this into PNG
@dot
digraph utmg {
    rankdir = LR;
    { node [label="",style=invis, height=0, width=0 ];
      ETV_Set_Data; ETV_Result; ETV_Data_Changed;
      ETM_Set_Data; ETM_Result; ETM_Changed;
      M_Dummy_1; M_Changed;
      AS_Modify;
    }
    { node [shape=box, fillcolor=lightyellow, style=filled,fontsize=26];
      EntityTreeView [label=":View"]; EntityTreeModel [label=":Model"]; Monitor [label=":Monitor"]; Akonadi_Server [label=":Akonadi"];
    }
    { node [style=invis];
      EntityTreeView_EOL; EntityTreeModel_EOL; Monitor_EOL; Akonadi_Server_EOL;
    }
    {
      { rank = same; ETV_Set_Data -> ETM_Set_Data [label="Set Data"]; }
      { rank = same; ETM_Result -> ETV_Result [label="False",arrowhead="vee", style="dashed"]; }
      { rank = same; ETM_Result -> M_Dummy_1 [style=invis]; }
      { rank = same; ETM_Set_Data -> AS_Modify [arrowhead="vee",taillabel="Modify Item", labeldistance=14.0, labelangle=10]; }
      { rank = same; M_Changed -> ETM_Changed [arrowhead="vee",label="Item Changed"]; }
      { rank = same; ETM_Changed -> ETV_Data_Changed [arrowhead="vee",label="Update View"]; }
    }

    // Dashed Vertical lines for object lifetimes.
    edge [style=dashed, arrowhead=none];
    { rank = same; EntityTreeView -> ETV_Set_Data -> ETV_Result -> ETV_Data_Changed -> EntityTreeView_EOL; }
    { rank = same; EntityTreeModel -> ETM_Set_Data -> ETM_Result -> ETM_Changed -> EntityTreeModel_EOL; }
    { rank = same; Monitor -> M_Dummy_1 -> M_Changed -> Monitor_EOL; }
    { rank = same; Akonadi_Server -> AS_Modify -> Akonadi_Server_EOL; }

    // Make sure top nodes are in a straight line.
    { EntityTreeView -> EntityTreeModel -> Monitor -> Akonadi_Server [style=invis]; }
    // And the bottom nodes.
    { EntityTreeView_EOL -> EntityTreeModel_EOL -> Monitor_EOL -> Akonadi_Server_EOL [style=invis]; }
}
@enddot
//-->

Similarly, in drag and drop operations, most models would update an internal data store and return <tt>true</tt> from dropMimeData if the drop is successful.

<!--
TODO: Render this to PNG
@dot
digraph utmg {
    rankdir = LR;
    { node [label="",style=invis, height=0, width=0 ];
      Left_Phantom; Left_Phantom_DropEvent;
      V_DropEvent; V_Result; V_Data_Changed; V_Dummy_1;
      M_DropMimeData; M_Result;
    }
    { node [shape=box, fillcolor=lightyellow, style=filled,fontsize=26];
      View [label=":View"]; Model [label=":Model"];
    }
    { node [style=invis];
       Left_Phantom_EOL;
       View_EOL; Model_EOL;
    }
    {
      Left_Phantom_DropEvent -> V_DropEvent [label="DropEvent"];
      V_DropEvent -> M_DropMimeData [label="DropMimeData"];
      M_Result -> V_Result [label="Success",arrowhead="vee", style="dashed"];
      V_Result -> V_Data_Changed [label="Update View \n[Success = True]"];
    }

    // Dashed Vertical lines for object lifetimes.
    edge [style=dashed, arrowhead=none];
    { rank = same; View -> V_DropEvent -> V_Result -> V_Dummy_1 -> V_Data_Changed -> View_EOL; }
    { rank = same; Model -> M_DropMimeData -> M_Result -> Model_EOL; }

    //Phantom line
    { rank= same; Left_Phantom -> Left_Phantom_DropEvent -> Left_Phantom_EOL [style=invis]; }

    // Make sure top nodes are in a straight line.
    {  Left_Phantom -> View -> Model [style=invis]; }
    // And the bottom nodes.
    {  Left_Phantom_EOL ->  View_EOL -> Model_EOL [style=invis]; }
}
@enddot
//--->

Akonadi models, for the same reason as above, always return false from dropMimeData. At the same time a suitable request is sent to the akonadi server to make the changes resulting from the drop (for example, moving or copying an entity, or adding a new entity to a collection etc). If that request is successful, the Akonadi::Monitor notifies the model that the data is changed and the model updates its internal store and notifies the view that the model data is changed.

<!--
TODO: Render this to PNG
@dot
digraph utmg {
    rankdir = LR;
    { node [label="",style=invis, height=0, width=0 ];
      Left_Phantom; Left_Phantom_DropEvent;
      ETV_DropEvent; ETV_Result; ETV_Data_Changed;
      ETM_DropMimeData; ETM_Result; ETM_Changed;
      M_Dummy_1; M_Changed;
      AS_Modify;
    }
    { node [shape=box, fillcolor=lightyellow, style=filled,fontsize=26];
      EntityTreeView [label=":View"];
      EntityTreeModel [label=":Model"];
      Monitor [label=":Monitor"];
      Akonadi_Server [label=":Akonadi"];
    }
    { node [style=invis];
      Left_Phantom_EOL;
      EntityTreeView_EOL; EntityTreeModel_EOL; Monitor_EOL; Akonadi_Server_EOL;
    }

    {
      { rank = same; Left_Phantom_DropEvent -> ETV_DropEvent [label="DropEvent"]; }
      { rank = same; ETV_DropEvent -> ETM_DropMimeData [label="Drop MimeData"]; }
      { rank = same; ETM_Result -> ETV_Result [label="False",arrowhead="vee", style="dashed"]; }
      { rank = same; ETM_Result -> M_Dummy_1 [style=invis]; }
      { rank = same; ETM_DropMimeData -> AS_Modify [arrowhead="vee",taillabel="Action", labeldistance=14.0, labelangle=10]; }
      { rank = same; M_Changed -> ETM_Changed [arrowhead="vee",label="Item Changed"]; }
      { rank = same; ETM_Changed -> ETV_Data_Changed [arrowhead="vee",label="Update View"]; }
    }

    // Dashed Vertical lines for object lifetimes.
    edge [style=dashed, arrowhead=none];
    { rank = same; EntityTreeView -> ETV_DropEvent -> ETV_Result -> ETV_Data_Changed -> EntityTreeView_EOL; }
    { rank = same; EntityTreeModel -> ETM_DropMimeData -> ETM_Result -> ETM_Changed -> EntityTreeModel_EOL; }
    { rank = same; Monitor -> M_Dummy_1 -> M_Changed -> Monitor_EOL; }
    { rank = same; Akonadi_Server -> AS_Modify -> Akonadi_Server_EOL; }

    //Phantom line
    { rank= same; Left_Phantom -> Left_Phantom_DropEvent -> Left_Phantom_EOL [style=invis]; }

    // Make sure top nodes are in a straight line.
    { Left_Phantom -> EntityTreeView -> EntityTreeModel -> Monitor -> Akonadi_Server [style=invis]; }
    // And the bottom nodes.
    { Left_Phantom_EOL -> EntityTreeView_EOL -> EntityTreeModel_EOL -> Monitor_EOL -> Akonadi_Server_EOL [style=invis]; }
}
@enddot
//-->
