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

#include "collectioncreatejob.h"
#include "protocolhelper_p.h"
#include "job_p.h"
#include "private/protocol_p.h"

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::CollectionCreateJobPrivate : public JobPrivate
{
public:
    CollectionCreateJobPrivate(CollectionCreateJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const Q_DECL_OVERRIDE;
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

    Protocol::CreateCollectionCommand cmd;
    cmd.setName(d->mCollection.name());
    cmd.setParent(ProtocolHelper::entityToScope(d->mCollection.parentCollection()));
    cmd.setMimeTypes(d->mCollection.contentMimeTypes());
    cmd.setRemoteId(d->mCollection.remoteId());
    cmd.setRemoteRevision(d->mCollection.remoteRevision());
    cmd.setIsVirtual(d->mCollection.isVirtual());
    cmd.setEnabled(d->mCollection.enabled());
    cmd.setDisplayPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListDisplay)));
    cmd.setSyncPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListDisplay)));
    cmd.setIndexPref(ProtocolHelper::listPreference(d->mCollection.localListPreference(Collection::ListIndex)));
    cmd.setCachePolicy(ProtocolHelper::cachePolicyToProtocol(d->mCollection.cachePolicy()));
    Protocol::Attributes attrs;
    const Akonadi::Attribute::List attrList = d->mCollection.attributes();
    for (Attribute *attr : attrList) {
        attrs.insert(attr->type(), attr->serialized());
    }
    cmd.setAttributes(attrs);

    d->sendCommand(cmd);
    emitWriteFinished();
}

Collection CollectionCreateJob::collection() const
{
    Q_D(const CollectionCreateJob);

    return d->mCollection;
}

bool CollectionCreateJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    Q_D(CollectionCreateJob);

    if (!response.isResponse()) {
        return Job::doHandleResponse(tag, response);
    }

    if (response.type() == Protocol::Command::FetchCollections) {
        Protocol::FetchCollectionsResponse resp(response);
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

    if (response.type() == Protocol::Command::CreateCollection) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}
