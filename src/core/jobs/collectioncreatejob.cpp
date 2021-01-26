/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectioncreatejob.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::CollectionCreateJobPrivate : public JobPrivate
{
public:
    explicit CollectionCreateJobPrivate(CollectionCreateJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const override;
    Collection mCollection;
};

QString Akonadi::CollectionCreateJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("Create collection: %1").arg(mCollection.id());
}

CollectionCreateJob::CollectionCreateJob(const Collection &collection, QObject *parent)
    : Job(new CollectionCreateJobPrivate(this), parent)
{
    Q_D(CollectionCreateJob);

    d->mCollection = collection;
}

CollectionCreateJob::~CollectionCreateJob()
{
}

void CollectionCreateJob::doStart()
{
    Q_D(CollectionCreateJob);
    if (d->mCollection.parentCollection().id() < 0 && d->mCollection.parentCollection().remoteId().isEmpty()) {
        setError(Unknown);
        setErrorText(i18n("Invalid parent"));
        emitResult();
        return;
    }

    auto cmd = Protocol::CreateCollectionCommandPtr::create();
    cmd->setName(d->mCollection.name());
    cmd->setParent(ProtocolHelper::entityToScope(d->mCollection.parentCollection()));
    cmd->setMimeTypes(d->mCollection.contentMimeTypes());
    cmd->setRemoteId(d->mCollection.remoteId());
    cmd->setRemoteRevision(d->mCollection.remoteRevision());
    cmd->setIsVirtual(d->mCollection.isVirtual());
    cmd->setEnabled(d->mCollection.enabled());
    cmd->setDisplayPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListDisplay)));
    cmd->setSyncPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListSync)));
    cmd->setIndexPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListIndex)));
    cmd->setCachePolicy(ProtocolHelper::cachePolicyToProtocol(d->mCollection.cachePolicy()));
    Protocol::Attributes attrs;
    const Akonadi::Attribute::List attrList = d->mCollection.attributes();
    for (Attribute *attr : attrList) {
        attrs.insert(attr->type(), attr->serialized());
    }
    cmd->setAttributes(attrs);

    d->sendCommand(cmd);
    emitWriteFinished();
}

Collection CollectionCreateJob::collection() const
{
    Q_D(const CollectionCreateJob);

    return d->mCollection;
}

bool CollectionCreateJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(CollectionCreateJob);

    if (!response->isResponse()) {
        return Job::doHandleResponse(tag, response);
    }

    if (response->type() == Protocol::Command::FetchCollections) {
        const auto &resp = Protocol::cmdCast<Protocol::FetchCollectionsResponse>(response);
        Collection col = ProtocolHelper::parseCollection(resp);
        if (!col.isValid()) {
            setError(Unknown);
            setErrorText(i18n("Failed to parse Collection from response"));
            return true;
        }
        col.setParentCollection(d->mCollection.parentCollection());
        col.setName(d->mCollection.name());
        col.setRemoteId(d->mCollection.remoteId());
        col.setRemoteRevision(d->mCollection.remoteRevision());
        col.setVirtual(d->mCollection.isVirtual());
        d->mCollection = col;
        return false;
    }

    if (response->type() == Protocol::Command::CreateCollection) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}
