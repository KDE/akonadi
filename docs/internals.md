# Internals documentation for Akonadi developers # {#internals}

## Lazy Model Population ##

> NOTE: This page is not part of the Akonadi API. It is provided as internal documentation for Akonadi maintainers. It was originally a blog post here: https://steveire.wordpress.com/2009/10/06/cache-invalidation-in-akonadi-models/

If using EntityTreeModel::LazyPopulation with your model, items will be fetched into the model when the collection they are a part of is selected. This ensures that the model is as sparsely populated as possible for performance reasons. As a consequence however, it is necessary to purge unused items from the model too. This is handled automatically when using an Akonadi::SelectionProxyModel.

The problem is knowing when to invalidate the cache. If no application is currently showing the contents of a Collection, there is no need for the Items in that Collection to be fetched, cached and kept up to date in the model. The effect we would like to achieve is to purge the Items in a Collection when those items are no longer shown anywhere in the application. Generally, that will mean that the Collection is not selected.

In Qt Model-View, the application data is stored in a model, and there may be one or more views attached to it displaying its contents. The model doesn’t have any knowledge of the views, and so it can’t know whether any particular Collection is selected, and purge its Iitems.

To solve this, we use the KSelectionProxyModel, which already has a lot of code for managing the selection a user makes in a view. A subclass, Akonadi::SelectionProxyModel implements a reference counting system which increments the refcount of a Collection when it is selected, and decrements it when deselected. If the reference count of a Collection goes down to zero, it is put in a queue to be purged. It is not purged immediately, but queued because if the user is clicking around several Collection in a short time, we don’t want to purge the Collections each click or we’d lose the benefit of the caching. Like other [similar optimisation techniques](http://qt-project.org/doc/qt-4.7/qobject.html#id-aa43c933-c869-42eb-af14-ff17b8304c96), this violates the object-oriented principle of modularity, but it is worth it for the benefit it brings. The effect can be seen in the akonadiconsole tool by not filtering out the items from the tree.

In the screenshots below I removed the filtering out of Items in the tree so that the fetching/purging can be seen. In real applications, the Items in the tree on the left would not be visible.

![bufferedcaching1](docs/images/bufferedcaching1.png)
"When a Collection is clicked, its Items are put into the model. The rest of the Collections have no items."

![bufferedcaching2](docs/images/bufferedcaching2.png)
"The Inbox Collection is selected, so its items are fetched. Personal Contacts is no longer selected, so it is put into a queue to be purged."

![bufferedcaching3](docs/images/bufferedcaching3.png)
"Select another Collection and its items are fetched too."

![bufferedcaching4](docs/images/bufferedcaching4.png)
"Another Collection is selected, pushing the Personal contacts out of the queue and purging them"

![bufferedcaching5](docs/images/bufferedcaching6.png)
"If the Collection is selected again, its Items are refetched"

For this example, I used a queue length of just two Collections, so that if a Collection was deselected two clicks ago, it will be purged. In real applications, a longer queue length will be used, but it’s harder to illustrate in screenshots. Another unrealistic part of this demo is that this feature will like be used in applications like KMail where Collections can contain tens of thousands of Items and fetching them is an expensive operation.

This feature should be totally invisible to users and even developers using Akonadi, but it should offset the main disadvantage of using a cache of Items in the EntityTreeModel.

*/

