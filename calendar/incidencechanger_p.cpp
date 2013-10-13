/*
  Copyright (C) 2013 Sérgio Martins <iamsergio@gmail.com>

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

#include "incidencechanger_p.h"
#include <kcalcore/incidence.h>

#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <QString>
#include <QSet>

using namespace Akonadi;

void IncidenceChanger::Private::loadCollections()
{
    if (isLoadingCollections()) {
        // Collections are already loading
        return;
    }

    m_collectionFetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection::root(),
                                                           Akonadi::CollectionFetchJob::Recursive);

    m_collectionFetchJob->fetchScope().setContentMimeTypes(KCalCore::Incidence::mimeTypes());
    connect(m_collectionFetchJob, SIGNAL(result(KJob*)), SLOT(onCollectionsLoaded(KJob*)));
    m_collectionFetchJob->start();
}

void IncidenceChanger::Private::onCollectionLoaded(KJob *job)
{
    m_collections.clear();

    if (job->error() == 0 && m_collectionFetchJob) {
        Q_ASSERT(job == m_collectionFetchJob);
        foreach (const Akonadi::Collection &collection, m_collectionFetchJob->collections()) {
            QSet<QString> mimeTypeSet = KCalCore::Incidence::mimeTypes().toSet();
            if (!mimeTypeSet.intersect(collection.contentMimeTypes().toSet()).isEmpty() &&
                collection.rights() & Akonadi::Collection::CanCreateItem) {
                m_collections << collection;
            }
        }
    } else {
        kError() << "Error loading collections:" << job->errorString();
    }

    m_collectionFetchJob = 0;
}

bool IncidenceChanger::Private::isLoadingCollections() const
{
    return m_collectionFetchJob != 0;
}

void IncidenceChanger::Private::continueCreatingIncidence(const Change::Ptr &change,
                                                          const KCalCore::Incidence::Ptr &incidence,
                                                          const Akonadi::Collection &collection)
{
    Q_ASSERT(change);

    mLastCollectionUsed = collection;

    Item item;
    item.setPayload<KCalCore::Incidence::Ptr>(incidence);
    item.setMimeType(incidence->mimeType());

    ItemCreateJob *createJob = new ItemCreateJob(item, collection, parentJob(change));
    mChangeForJob.insert(createJob, change);

    if (mBatchOperationInProgress) {
        AtomicOperation *atomic = mAtomicOperations[mLatestAtomicOperationId];
        Q_ASSERT(atomic);
        atomic->addChange(change);
    }

    // QueuedConnection because of possible sync exec calls.
    connect(createJob, SIGNAL(result(KJob*)),
            SLOT(handleCreateJobResult(KJob*)), Qt::QueuedConnection );

    mChangeById.insert(change->id, change);
}


