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
#include "utils_p.h"
#include <kcalcore/incidence.h>

#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <QString>
#include <QDialog>
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

void IncidenceChanger::Private::step2CreateIncidence(const Change::Ptr &change,
                                                     const Akonadi::Collection &collection)
{
    Q_ASSERT(change);

    mLastCollectionUsed = collection;

    ItemCreateJob *createJob = new ItemCreateJob(change->newItem, collection, parentJob(change));
    mChangeForJob.insert(createJob, change);

    if (mBatchOperationInProgress) {
        AtomicOperation *atomic = mAtomicOperations[mLatestAtomicOperationId];
        Q_ASSERT(atomic);
        atomic->addChange(change);
    }

    // QueuedConnection because of possible sync exec calls.
    connect(createJob, SIGNAL(result(KJob*)),
            SLOT(handleCreateJobResult(KJob*)), Qt::QueuedConnection);

    mChangeById.insert(change->id, change);
}


void IncidenceChanger::Private::step1DetermineDestinationCollection(const Change::Ptr &change,
                                                                    const Akonadi::Collection &collection)
{
    KCalCore::Incidence::Ptr incidence = CalendarUtils::incidence(change->newItem);
    Collection collectionToUse;
    QWidget *parent = change->parentWidget.data();
    if (collection.isValid() && hasRights(collection, ChangeTypeCreate)) {
        // The collection passed always has priority
        collectionToUse = collection;
    } else {
        switch (mDestinationPolicy) {
        case DestinationPolicyDefault:
            if (mDefaultCollection.isValid() && hasRights(mDefaultCollection, ChangeTypeCreate)) {
                collectionToUse = mDefaultCollection;
                break;
            }
            kWarning() << "Destination policy is to use the default collection."
                       << "But it's invalid or doesn't have proper ACLs."
                       << "isValid = "  << mDefaultCollection.isValid()
                       << "has ACLs = " << hasRights(mDefaultCollection, ChangeTypeCreate);
            // else fallthrough, and ask the user.
        case DestinationPolicyAsk:
        {
            int dialogCode;
            const QStringList mimeTypes(incidence->mimeType());
            collectionToUse = CalendarUtils::selectCollection(parent, dialogCode /*by-ref*/, mimeTypes, mDefaultCollection);
            if (dialogCode != QDialog::Accepted) {
                kDebug() << "User canceled collection choosing";
                change->resultCode = ResultCodeUserCanceled;
                cancelTransaction();
                return;
            }

            if (collectionToUse.isValid() && !hasRights(collectionToUse, ChangeTypeCreate)) {
                kWarning() << "No ACLs for incidence creation";
                const QString errorMessage = showErrorDialog(ResultCodePermissions, parent);
                change->resultCode = ResultCodePermissions;
                change->errorString = errorMessage;
                cancelTransaction();
                return;
            }

            // TODO: add unit test for these two situations after reviewing API
            if (!collectionToUse.isValid()) {
                kError() << "Invalid collection selected. Can't create incidence.";
                change->resultCode = ResultCodeInvalidUserCollection;
                const QString errorString = showErrorDialog(ResultCodeInvalidUserCollection, parent);
                change->errorString = errorString;
                cancelTransaction();
                return;
            }
        }
            break;
        case DestinationPolicyNeverAsk:
        {
            const bool hasRights = this->hasRights(mDefaultCollection, ChangeTypeCreate);
            if (mDefaultCollection.isValid() && hasRights) {
                collectionToUse = mDefaultCollection;
            } else {
                const QString errorString = showErrorDialog(ResultCodeInvalidDefaultCollection, parent);
                kError() << errorString << "; rights are " << hasRights;
                change->resultCode = hasRights ? ResultCodeInvalidDefaultCollection :
                                                 ResultCodePermissions;
                change->errorString = errorString;
                cancelTransaction();
                return;
            }
            break;
        }

        default:
            // Never happens
            Q_ASSERT_X(false, "createIncidence()", "unknown destination policy");
            cancelTransaction();
            return;
        }
    }

    step2CreateIncidence(change, collectionToUse);
}
