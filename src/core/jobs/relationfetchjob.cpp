/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "relationfetchjob.h"
#include "job_p.h"
#include "relation.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"
#include <QTimer>

using namespace Akonadi;

class Akonadi::RelationFetchJobPrivate : public JobPrivate
{
public:
    RelationFetchJobPrivate(RelationFetchJob *parent)
        : JobPrivate(parent)
        , mEmitTimer(nullptr)
    {
    }

    void init()
    {
        Q_Q(RelationFetchJob);
        mEmitTimer = new QTimer(q);
        mEmitTimer->setSingleShot(true);
        mEmitTimer->setInterval(100);
        q->connect(mEmitTimer, SIGNAL(timeout()), q, SLOT(timeout()));
    }

    void aboutToFinish() override {
        timeout();
    }

    void timeout()
    {
        Q_Q(RelationFetchJob);
        mEmitTimer->stop(); // in case we are called by result()
        if (!mPendingRelations.isEmpty()) {
            if (!q->error()) {
                emit q->relationsReceived(mPendingRelations);
            }
            mPendingRelations.clear();
        }
    }

    Q_DECLARE_PUBLIC(RelationFetchJob)

    Relation::List mResultRelations;
    Relation::List mPendingRelations; // relation pending for emitting itemsReceived()
    QTimer *mEmitTimer = nullptr;
    QVector<QByteArray> mTypes;
    QString mResource;
    Relation mRequestedRelation;
};

RelationFetchJob::RelationFetchJob(const Relation &relation, QObject *parent)
    : Job(new RelationFetchJobPrivate(this), parent)
{
    Q_D(RelationFetchJob);
    d->init();
    d->mRequestedRelation = relation;
}

RelationFetchJob::RelationFetchJob(const QVector<QByteArray> &types, QObject *parent)
    : Job(new RelationFetchJobPrivate(this), parent)
{
    Q_D(RelationFetchJob);
    d->init();
    d->mTypes = types;
}

void RelationFetchJob::doStart()
{
    Q_D(RelationFetchJob);

    d->sendCommand(Protocol::FetchRelationsCommandPtr::create(
                       d->mRequestedRelation.left().id(),
                       d->mRequestedRelation.right().id(),
                       (d->mTypes.isEmpty() && !d->mRequestedRelation.type().isEmpty()) ? QVector<QByteArray>() << d->mRequestedRelation.type() : d->mTypes,
                       d->mResource));
}

bool RelationFetchJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(RelationFetchJob);

    if (!response->isResponse() || response->type() != Protocol::Command::FetchRelations) {
        return Job::doHandleResponse(tag, response);
    }

    const Relation rel = ProtocolHelper::parseRelationFetchResult(
        Protocol::cmdCast<Protocol::FetchRelationsResponse>(response));
    // Invalid response means there will be no more responses
    if (!rel.isValid()) {
        return true;
    }

    d->mResultRelations.append(rel);
    d->mPendingRelations.append(rel);
    if (!d->mEmitTimer->isActive()) {
        d->mEmitTimer->start();
    }
    return false;
}

Relation::List RelationFetchJob::relations() const
{
    Q_D(const RelationFetchJob);
    return d->mResultRelations;
}

void RelationFetchJob::setResource(const QString &identifier)
{
    Q_D(RelationFetchJob);
    d->mResource = identifier;
}

#include "moc_relationfetchjob.cpp"
