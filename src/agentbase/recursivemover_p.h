/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RECURSIVEMOVER_P_H
#define AKONADI_RECURSIVEMOVER_P_H

#include "agentbase_p.h"
#include "collection.h"
#include "item.h"

#include <KCompositeJob>

namespace Akonadi
{
/**
 * Helper class for expanding inter-resource collection moves inside ResourceBase.
 *
 * @note This is intentionally not an Akonadi::Job since we don't need autostarting
 * here.
 */
class RecursiveMover : public KCompositeJob
{
    Q_OBJECT
public:
    explicit RecursiveMover(AgentBasePrivate *parent);

    /// Set the collection that is actually moved.
    void setCollection(const Akonadi::Collection &collection, const Akonadi::Collection &parentCollection);

    void start() override;

    /// Call once the last replayed change has been processed.
    void changeProcessed();

public Q_SLOTS:
    /// Trigger the next change replay, will call emitResult() once everything has been replayed
    void replayNext();

private:
    void replayNextCollection();
    void replayNextItem();

private Q_SLOTS:
    void collectionListResult(KJob *job);
    void collectionFetchResult(KJob *job);
    void itemListResult(KJob *job);
    void itemFetchResult(KJob *job);

private:
    AgentBasePrivate *const m_agentBase;
    Collection m_movedCollection;
    /// sorted queue of collections still to be processed
    Collection::List m_pendingCollections;
    /// holds up-to-date full collection objects, used for e.g. having proper parent collections for collectionAdded
    QHash<Collection::Id, Collection> m_collections;
    Item::List m_pendingItems;

    Collection m_currentCollection;
    Item m_currentItem;

    enum CurrentAction { None, AddCollection, AddItem } m_currentAction;

    int m_runningJobs = 0;
    bool m_pendingReplay = false;
};

}

Q_DECLARE_METATYPE(Akonadi::RecursiveMover *)

#endif // AKONADI_RECURSIVEMOVER_P_H
