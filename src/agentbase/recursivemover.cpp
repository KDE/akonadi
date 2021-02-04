/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "recursivemover_p.h"

using namespace Akonadi;

RecursiveMover::RecursiveMover(AgentBasePrivate *parent)
    : KCompositeJob(parent)
    , m_agentBase(parent)
    , m_currentAction(None)
{
}

void RecursiveMover::start()
{
    Q_ASSERT(receivers(SIGNAL(result(KJob *))));

    auto job = new CollectionFetchJob(m_movedCollection, CollectionFetchJob::Recursive, this);
    connect(job, &CollectionFetchJob::finished, this, &RecursiveMover::collectionListResult);
    addSubjob(job);
    ++m_runningJobs;
}

void RecursiveMover::setCollection(const Collection &collection, const Collection &parentCollection)
{
    m_movedCollection = collection;
    m_collections.insert(collection.id(), m_movedCollection);
    m_collections.insert(parentCollection.id(), parentCollection);
}

void RecursiveMover::collectionListResult(KJob *job)
{
    Q_ASSERT(m_pendingCollections.isEmpty());
    --m_runningJobs;

    if (job->error()) {
        return; // error handling is in the base class
    }

    // build a parent -> children map for the following topological sorting
    // while we are iterating anyway, also fill m_collections here
    auto fetchJob = qobject_cast<CollectionFetchJob *>(job);
    QHash<Collection::Id, Collection::List> colTree;
    const Akonadi::Collection::List lstCol = fetchJob->collections();
    for (const Collection &col : lstCol) {
        colTree[col.parentCollection().id()] << col;
        m_collections.insert(col.id(), col);
    }

    // topological sort; BFS traversal of the tree
    m_pendingCollections.push_back(m_movedCollection);
    QQueue<Collection> toBeProcessed;
    toBeProcessed.enqueue(m_movedCollection);
    while (!toBeProcessed.isEmpty()) {
        const Collection col = toBeProcessed.dequeue();
        const Collection::List children = colTree.value(col.id());
        if (children.isEmpty()) {
            continue;
        }
        m_pendingCollections += children;
        for (const Collection &child : children) {
            toBeProcessed.enqueue(child);
        }
    }

    replayNextCollection();
}

void RecursiveMover::collectionFetchResult(KJob *job)
{
    Q_ASSERT(m_currentCollection.isValid());
    --m_runningJobs;

    if (job->error()) {
        return; // error handling is in the base class
    }

    auto fetchJob = qobject_cast<CollectionFetchJob *>(job);
    if (fetchJob->collections().size() == 1) {
        m_currentCollection = fetchJob->collections().at(0);
        m_currentCollection.setParentCollection(m_collections.value(m_currentCollection.parentCollection().id()));
        m_collections.insert(m_currentCollection.id(), m_currentCollection);
    } else {
        // already deleted, move on
    }

    if (!m_runningJobs && m_pendingReplay) {
        replayNext();
    }
}

void RecursiveMover::itemListResult(KJob *job)
{
    --m_runningJobs;

    if (job->error()) {
        return; // error handling is in the base class
    }
    const Akonadi::Item::List lstItems = qobject_cast<ItemFetchJob *>(job)->items();
    for (const Item &item : lstItems) {
        if (item.remoteId().isEmpty()) {
            m_pendingItems.push_back(item);
        }
    }

    if (!m_runningJobs && m_pendingReplay) {
        replayNext();
    }
}

void RecursiveMover::itemFetchResult(KJob *job)
{
    Q_ASSERT(m_currentAction == None);
    --m_runningJobs;

    if (job->error()) {
        return; // error handling is in the base class
    }

    auto fetchJob = qobject_cast<ItemFetchJob *>(job);
    if (fetchJob->items().size() == 1) {
        m_currentAction = AddItem;
        m_agentBase->itemAdded(fetchJob->items().at(0), m_currentCollection);
    } else {
        // deleted since we started, skip
        m_currentItem = Item();
        replayNextItem();
    }
}

void RecursiveMover::replayNextCollection()
{
    if (!m_pendingCollections.isEmpty()) {
        m_currentCollection = m_pendingCollections.takeFirst();
        auto job = new ItemFetchJob(m_currentCollection, this);
        connect(job, &ItemFetchJob::result, this, &RecursiveMover::itemListResult);
        addSubjob(job);
        ++m_runningJobs;

        if (m_currentCollection.remoteId().isEmpty()) {
            Q_ASSERT(m_currentAction == None);
            m_currentAction = AddCollection;
            m_agentBase->collectionAdded(m_currentCollection, m_collections.value(m_currentCollection.parentCollection().id()));
            return;
        } else {
            // replayNextItem(); - but waiting for the fetch job to finish first
            m_pendingReplay = true;
            return;
        }
    } else {
        // nothing left to do
        emitResult();
    }
}

void RecursiveMover::replayNextItem()
{
    Q_ASSERT(m_currentCollection.isValid());
    if (m_pendingItems.isEmpty()) {
        replayNextCollection(); // all items processed here
        return;
    } else {
        Q_ASSERT(m_currentAction == None);
        m_currentItem = m_pendingItems.takeFirst();
        auto job = new ItemFetchJob(m_currentItem, this);
        job->fetchScope().fetchFullPayload();
        connect(job, &ItemFetchJob::result, this, &RecursiveMover::itemFetchResult);
        addSubjob(job);
        ++m_runningJobs;
    }
}

void RecursiveMover::changeProcessed()
{
    Q_ASSERT(m_currentAction != None);

    if (m_currentAction == AddCollection) {
        Q_ASSERT(m_currentCollection.isValid());
        auto job = new CollectionFetchJob(m_currentCollection, CollectionFetchJob::Base, this);
        job->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
        connect(job, &CollectionFetchJob::result, this, &RecursiveMover::collectionFetchResult);
        addSubjob(job);
        ++m_runningJobs;
    }

    m_currentAction = None;
}

void RecursiveMover::replayNext()
{
    // wait for runnings jobs to finish before actually doing the replay
    if (m_runningJobs) {
        m_pendingReplay = true;
        return;
    }

    m_pendingReplay = false;

    if (m_currentCollection.isValid()) {
        replayNextItem();
    } else {
        replayNextCollection();
    }
}

#include "moc_recursivemover_p.cpp"
