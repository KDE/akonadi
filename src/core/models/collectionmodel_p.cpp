/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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

//@cond PRIVATE

#include "collectionmodel_p.h"
#include "collectionmodel.h"
#include "collectionutils.h"

#include "collectionfetchjob.h"
#include "collectionstatistics.h"
#include "collectionstatisticsjob.h"
#include "monitor.h"
#include "session.h"
#include "collectionfetchscope.h"

#include "akonadicore_debug.h"

#include <kjob.h>
#include <kiconloader.h>

#include <QCoreApplication>
#include <QtCore/QTimer>

using namespace Akonadi;

void CollectionModelPrivate::collectionRemoved(const Akonadi::Collection &collection)
{
    Q_Q(CollectionModel);
    QModelIndex colIndex = indexForId(collection.id());
    if (colIndex.isValid()) {
        QModelIndex parentIndex = q->parent(colIndex);
        // collection is still somewhere in the hierarchy
        removeRowFromModel(colIndex.row(), parentIndex);
    } else {
        if (collections.contains(collection.id())) {
            // collection is orphan, ie. the parent has been removed already
            collections.remove(collection.id());
            childCollections.remove(collection.id());
        }
    }
}

void CollectionModelPrivate::collectionChanged(const Akonadi::Collection &collection)
{
    Q_Q(CollectionModel);
    // What kind of change is it ?
    Collection::Id oldParentId = collections.value(collection.id()).parentCollection().id();
    Collection::Id newParentId = collection.parentCollection().id();
    if (newParentId !=  oldParentId && oldParentId >= 0) {   // It's a move
        removeRowFromModel(indexForId(collections[collection.id()].id()).row(), indexForId(oldParentId));
        Collection newParent;
        if (newParentId == Collection::root().id()) {
            newParent = Collection::root();
        } else {
            newParent = collections.value(newParentId);
        }
        CollectionFetchJob *job = new CollectionFetchJob(newParent, CollectionFetchJob::Recursive, session);
        job->fetchScope().setListFilter(unsubscribed ? CollectionFetchScope::NoFilter : CollectionFetchScope::Enabled);
        job->fetchScope().setIncludeStatistics(fetchStatistics);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                   q, SLOT(collectionsChanged(Akonadi::Collection::List)));
        q->connect(job, SIGNAL(result(KJob*)),
                   q, SLOT(listDone(KJob*)));

    } else { // It's a simple change
        CollectionFetchJob *job = new CollectionFetchJob(collection, CollectionFetchJob::Base, session);
        job->fetchScope().setListFilter(unsubscribed ? CollectionFetchScope::NoFilter : CollectionFetchScope::Enabled);
        job->fetchScope().setIncludeStatistics(fetchStatistics);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                   q, SLOT(collectionsChanged(Akonadi::Collection::List)));
        q->connect(job, SIGNAL(result(KJob*)),
                   q, SLOT(listDone(KJob*)));
    }

}

void CollectionModelPrivate::updateDone(KJob *job)
{
    if (job->error()) {
        // TODO: handle job errors
        qCWarning(AKONADICORE_LOG) << "Job error:" << job->errorString();
    } else {
        CollectionStatisticsJob *csjob = static_cast<CollectionStatisticsJob *>(job);
        Collection result = csjob->collection();
        collectionStatisticsChanged(result.id(), csjob->statistics());
    }
}

void CollectionModelPrivate::collectionStatisticsChanged(Collection::Id collection,
        const Akonadi::CollectionStatistics &statistics)
{
    Q_Q(CollectionModel);

    if (!collections.contains(collection)) {
        qCWarning(AKONADICORE_LOG) << "Got statistics response for non-existing collection:" << collection;
    } else {
        collections[collection].setStatistics(statistics);

        Collection col = collections.value(collection);
        QModelIndex startIndex = indexForId(col.id());
        QModelIndex endIndex = indexForId(col.id(), q->columnCount(q->parent(startIndex)) - 1);
        emit q->dataChanged(startIndex, endIndex);
    }
}

void CollectionModelPrivate::listDone(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Job error: " << job->errorString() << endl;
    }
}

void CollectionModelPrivate::editDone(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Edit failed: " << job->errorString();
    }
}

void CollectionModelPrivate::dropResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Paste failed:" << job->errorString();
        // TODO: error handling
    }
}

void CollectionModelPrivate::collectionsChanged(const Collection::List &cols)
{
    Q_Q(CollectionModel);

    foreach (Collection col, cols) {    //krazy:exclude=foreach non-const is needed here
        if (collections.contains(col.id())) {
            // If the collection is already known to the model, we simply update it...
            col.setStatistics(collections.value(col.id()).statistics());
            collections[col.id()] = col;
            QModelIndex startIndex = indexForId(col.id());
            QModelIndex endIndex = indexForId(col.id(), q->columnCount(q->parent(startIndex)) - 1);
            emit q->dataChanged(startIndex, endIndex);
            continue;
        }
        // ... otherwise we add it to the set of collections we need to handle.
        m_newChildCollections[col.parentCollection().id()].append(col.id());
        m_newCollections.insert(col.id(), col);
    }

    // Handle the collections in m_newChildCollections. If the collections
    // parent is already in the model, the collection can be added to the model.
    // Otherwise it is persisted until it has a valid parent in the model.
    int currentSize = m_newChildCollections.size();
    int lastSize = -1;

    while (currentSize > 0) {
        lastSize = currentSize;

        QMutableHashIterator< Collection::Id, QVector< Collection::Id > > i(m_newChildCollections);
        while (i.hasNext()) {
            i.next();

            // the key is the parent of new collections. It may itself also be new,
            // but that will be handled later.
            Collection::Id colId = i.key();

            QVector< Collection::Id > newChildCols = i.value();
            int newChildCount = newChildCols.size();
//       if ( newChildCount == 0 )
//       {
//         // Sanity check.
//         qCDebug(AKONADICORE_LOG) << "No new child collections have been added to the collection:" << colId;
//         i.remove();
//         currentSize--;
//         break;
//       }

            if (collections.contains(colId) || colId == Collection::root().id()) {
                QModelIndex parentIndex = indexForId(colId);
                int currentChildCount = childCollections.value(colId).size();

                q->beginInsertRows(parentIndex,
                                   currentChildCount,      // Start index is at the end of existing collections.
                                   currentChildCount + newChildCount - 1);  // End index is the result of the insertion.

                foreach (Collection::Id id, newChildCols) {
                    Collection c = m_newCollections.take(id);
                    collections.insert(id, c);
                }

                childCollections[colId] << newChildCols;
                q->endInsertRows();
                i.remove();
                currentSize--;
                break;
            }
        }

        // We iterated through once without adding any more collections to the model.
        if (currentSize == lastSize) {
            // The remaining collections in the list do not have a valid parent in the model yet. They
            // might arrive in the next batch from the monitor, so they're still in m_newCollections
            // and m_newChildCollections.
            qCDebug(AKONADICORE_LOG) << "Some collections did not have a parent in the model yet!";
            break;
        }
    }
}

QModelIndex CollectionModelPrivate::indexForId(Collection::Id id, int column) const
{
    Q_Q(const CollectionModel);
    if (!collections.contains(id)) {
        return QModelIndex();
    }

    Collection::Id parentId = collections.value(id).parentCollection().id();
    // check if parent still exist or if this is an orphan collection
    if (parentId != Collection::root().id() && !collections.contains(parentId)) {
        return QModelIndex();
    }

    QVector<Collection::Id> list = childCollections.value(parentId);
    int row = list.indexOf(id);

    if (row >= 0) {
        return q->createIndex(row, column, reinterpret_cast<void *>(collections.value(list.at(row)).id()));
    }
    return QModelIndex();
}

bool CollectionModelPrivate::removeRowFromModel(int row, const QModelIndex &parent)
{
    Q_Q(CollectionModel);
    QVector<Collection::Id> list;
    Collection parentCol;
    if (parent.isValid()) {
        parentCol = collections.value(parent.internalId());
        Q_ASSERT(parentCol.id() == static_cast<qint64>(parent.internalId()));
        list = childCollections.value(parentCol.id());
    } else {
        parentCol = Collection::root();
        list = childCollections.value(Collection::root().id());
    }
    if (row < 0 || row  >= list.size()) {
        qCWarning(AKONADICORE_LOG) << "Index out of bounds:" << row << " parent:" << parentCol.id();
        return false;
    }

    q->beginRemoveRows(parent, row, row);
    const Collection::Id delColId = list[row];
    list.remove(row);
    foreach (Collection::Id childColId, childCollections[delColId]) {
        collections.remove(childColId);
    }
    collections.remove(delColId);
    childCollections.remove(delColId);   // remove children of deleted collection
    childCollections.insert(parentCol.id(), list);   // update children of parent
    q->endRemoveRows();

    return true;
}

bool CollectionModelPrivate::supportsContentType(const QModelIndex &index, const QStringList &contentTypes)
{
    if (!index.isValid()) {
        return false;
    }
    Collection col = collections.value(index.internalId());
    Q_ASSERT(col.isValid());
    QStringList ct = col.contentMimeTypes();
    foreach (const QString &a, ct) {
        if (contentTypes.contains(a)) {
            return true;
        }
    }
    return false;
}

void CollectionModelPrivate::init()
{
    Q_Q(CollectionModel);

    session = new Session(QCoreApplication::instance()->applicationName().toUtf8()
                          + QByteArray("-CollectionModel-") + QByteArray::number(qrand()), q);
    QTimer::singleShot(0, q, SLOT(startFirstListJob()));

    // monitor collection changes
    monitor = new Monitor();
    monitor->setCollectionMonitored(Collection::root());
    monitor->fetchCollection(true);

    // ### Hack to get the kmail resource folder icons
    KIconLoader::global()->addAppDir(QStringLiteral("kmail"));
    KIconLoader::global()->addAppDir(QStringLiteral("kdepim"));

    // monitor collection changes
    q->connect(monitor, SIGNAL(collectionChanged(Akonadi::Collection)),
               q, SLOT(collectionChanged(Akonadi::Collection)));
    q->connect(monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
               q, SLOT(collectionChanged(Akonadi::Collection)));
    q->connect(monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
               q, SLOT(collectionRemoved(Akonadi::Collection)));
    q->connect(monitor, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)),
               q, SLOT(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)));
}

QIcon CollectionModelPrivate::iconForCollection(const Collection &col) const
{
    // Reset the cache when icon theme changes
    if (mIconThemeName != QIcon::themeName()) {
        mIconThemeName = QIcon::themeName();
        mIconCache.clear();
    }

    QString iconName;
    if (col.hasAttribute<EntityDisplayAttribute>()) {
        iconName = col.attribute<EntityDisplayAttribute>()->iconName();
    }
    if (iconName.isEmpty()) {
        iconName = CollectionUtils::defaultIconName(col);
    }

    QIcon &icon = mIconCache[iconName];
    if (icon.isNull()) {
        icon = QIcon::fromTheme(iconName);
    }
    return icon;
}

void CollectionModelPrivate::startFirstListJob()
{
    Q_Q(CollectionModel);

    // start a list job
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, session);
    job->fetchScope().setListFilter(unsubscribed ? CollectionFetchScope::NoFilter : CollectionFetchScope::Enabled);
    job->fetchScope().setIncludeStatistics(fetchStatistics);
    q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
               q, SLOT(collectionsChanged(Akonadi::Collection::List)));
    q->connect(job, SIGNAL(result(KJob*)), q, SLOT(listDone(KJob*)));
}

//@endcond
