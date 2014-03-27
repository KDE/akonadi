/*
    Copyright (c) 2013 David Faure <faure@kde.org>

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

#include "specialcollectionsdiscoveryjob.h"
#include "specialcollectionattribute.h"
#include "collectionfetchscope.h"
#include "collectionfetchjob.h"
#include <QStringList>

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

    SpecialCollections *mSpecialCollections;
    QStringList mMimeTypes;
};

Akonadi::SpecialCollectionsDiscoveryJob::SpecialCollectionsDiscoveryJob(SpecialCollections *collections, const QStringList &mimeTypes, QObject *parent)
    : KCompositeJob(parent)
    , d(new SpecialCollectionsDiscoveryJobPrivate(collections, mimeTypes))
{
}

Akonadi::SpecialCollectionsDiscoveryJob::~SpecialCollectionsDiscoveryJob()
{
    delete d;
}

void Akonadi::SpecialCollectionsDiscoveryJob::start()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    job->fetchScope().setContentMimeTypes(d->mMimeTypes);
    addSubjob(job);
}

void Akonadi::SpecialCollectionsDiscoveryJob::slotResult(KJob *job)
{
    if (job->error()) {
        qWarning() << job->errorString();
        return;
    }
    Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    foreach (const Akonadi::Collection &collection, fetchJob->collections()) {
        if (collection.hasAttribute<SpecialCollectionAttribute>()) {
            d->mSpecialCollections->registerCollection(collection.attribute<SpecialCollectionAttribute>()->collectionType(), collection);
        }
    }
    emitResult();
}
