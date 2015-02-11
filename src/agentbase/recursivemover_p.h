/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_RECURSIVEMOVER_P_H
#define AKONADI_RECURSIVEMOVER_P_H

#include "item.h"
#include "collection.h"
#include "item.h"
#include "agentbase_p.h"

#include <kcompositejob.h>

namespace Akonadi {

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

    virtual void start() Q_DECL_OVERRIDE;

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
    AgentBasePrivate *m_agentBase;
    Collection m_movedCollection;
    /// sorted queue of collections still to be processed
    Collection::List m_pendingCollections;
    /// holds up-to-date full collection objects, used for e.g. having proper parent collections for collectionAdded
    QHash<Collection::Id, Collection> m_collections;
    Item::List m_pendingItems;

    Collection m_currentCollection;
    Item m_currentItem;

    enum CurrentAction {
        None,
        AddCollection,
        AddItem
    } m_currentAction;

    int m_runningJobs;
    bool m_pendingReplay;
};

}

Q_DECLARE_METATYPE(Akonadi::RecursiveMover *)

#endif // AKONADI_RECURSIVEMOVER_P_H
