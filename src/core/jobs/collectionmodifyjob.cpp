/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "collectionmodifyjob.h"

#include "changemediator_p.h"
#include "collection_p.h"
#include "collectionstatistics.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"
#include "persistentsearchattribute.h"

using namespace Akonadi;

class Akonadi::CollectionModifyJobPrivate : public JobPrivate
{
public:
    explicit CollectionModifyJobPrivate(CollectionModifyJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const override
    {
        return QStringLiteral("Collection Id %1").arg(mCollection.id());
    }

    Collection mCollection;
};

CollectionModifyJob::CollectionModifyJob(const Collection &collection, QObject *parent)
    : Job(new CollectionModifyJobPrivate(this), parent)
{
    Q_D(CollectionModifyJob);
    d->mCollection = collection;
}

CollectionModifyJob::~CollectionModifyJob()
{
}

void CollectionModifyJob::doStart()
{
    Q_D(CollectionModifyJob);

    Protocol::ModifyCollectionCommandPtr cmd;
    try {
        cmd = Protocol::ModifyCollectionCommandPtr::create(ProtocolHelper::entityToScope(d->mCollection));
    } catch (const std::exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }

    if (d->mCollection.d_ptr->contentTypesChanged) {
        cmd->setMimeTypes(d->mCollection.contentMimeTypes());
    }
    if (d->mCollection.parentCollection().id() >= 0) {
        cmd->setParentId(d->mCollection.parentCollection().id());
    }
    const QString &collectionName = d->mCollection.name();
    if (!collectionName.isEmpty()) {
        cmd->setName(collectionName);
    }
    if (!d->mCollection.remoteId().isNull()) {
        cmd->setRemoteId(d->mCollection.remoteId());
    }
    if (!d->mCollection.remoteRevision().isNull()) {
        cmd->setRemoteRevision(d->mCollection.remoteRevision());
    }
    if (d->mCollection.d_ptr->cachePolicyChanged) {
        cmd->setCachePolicy(ProtocolHelper::cachePolicyToProtocol(d->mCollection.cachePolicy()));
    }
    if (d->mCollection.d_ptr->enabledChanged) {
        cmd->setEnabled(d->mCollection.enabled());
    }
    if (d->mCollection.d_ptr->listPreferenceChanged) {
        cmd->setDisplayPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListDisplay)));
        cmd->setSyncPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListSync)));
        cmd->setIndexPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListIndex)));
    }
    if (d->mCollection.d_ptr->mAttributeStorage.hasModifiedAttributes()) {
        cmd->setAttributes(ProtocolHelper::attributesToProtocol(d->mCollection.d_ptr->mAttributeStorage.modifiedAttributes()));
    }
    if (auto *attr = d->mCollection.attribute<Akonadi::PersistentSearchAttribute>()) {
        cmd->setPersistentSearchCollections(attr->queryCollections());
        cmd->setPersistentSearchQuery(attr->queryString());
        cmd->setPersistentSearchRecursive(attr->isRecursive());
        cmd->setPersistentSearchRemote(attr->isRemoteSearchEnabled());
    }
    if (!d->mCollection.d_ptr->mAttributeStorage.deletedAttributes().empty()) {
        cmd->setRemovedAttributes(d->mCollection.d_ptr->mAttributeStorage.deletedAttributes());
    }

    if (cmd->modifiedParts() == Protocol::ModifyCollectionCommand::None) {
        emitResult();
        return;
    }

    d->sendCommand(cmd);

    ChangeMediator::invalidateCollection(d->mCollection);
}

bool CollectionModifyJob::doHandleResponse(qint64 tag, const Akonadi::Protocol::CommandPtr &response)
{
    Q_D(CollectionModifyJob);

    if (!response->isResponse() || response->type() != Protocol::Command::ModifyCollection) {
        return Job::doHandleResponse(tag, response);
    }

    d->mCollection.d_ptr->resetChangeLog();
    return true;
}

Collection CollectionModifyJob::collection() const
{
    const Q_D(CollectionModifyJob);
    return d->mCollection;
}
