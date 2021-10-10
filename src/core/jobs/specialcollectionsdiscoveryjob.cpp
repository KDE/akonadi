/*
    SPDX-FileCopyrightText: 2013 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "specialcollectionsdiscoveryjob.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "specialcollectionattribute.h"
#include <QStringList>

#include "akonadicore_debug.h"

using namespace Akonadi;

/**
  @internal
*/
class Akonadi::SpecialCollectionsDiscoveryJobPrivate
{
public:
    SpecialCollectionsDiscoveryJobPrivate(SpecialCollections *collections, const QStringList &mimeTypes)
        : mSpecialCollections(collections)
        , mMimeTypes(mimeTypes)
    {
    }

    SpecialCollections *const mSpecialCollections;
    const QStringList mMimeTypes;
};

Akonadi::SpecialCollectionsDiscoveryJob::SpecialCollectionsDiscoveryJob(SpecialCollections *collections, const QStringList &mimeTypes, QObject *parent)
    : KCompositeJob(parent)
    , d(new SpecialCollectionsDiscoveryJobPrivate(collections, mimeTypes))
{
}

Akonadi::SpecialCollectionsDiscoveryJob::~SpecialCollectionsDiscoveryJob() = default;

void Akonadi::SpecialCollectionsDiscoveryJob::start()
{
    auto job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    job->fetchScope().setContentMimeTypes(d->mMimeTypes);
    addSubjob(job);
}

void Akonadi::SpecialCollectionsDiscoveryJob::slotResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorString();
        return;
    }
    auto fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    const Akonadi::Collection::List lstCollections = fetchJob->collections();
    for (const Akonadi::Collection &collection : lstCollections) {
        if (collection.hasAttribute<SpecialCollectionAttribute>()) {
            d->mSpecialCollections->registerCollection(collection.attribute<SpecialCollectionAttribute>()->collectionType(), collection);
        }
    }
    emitResult();
}
