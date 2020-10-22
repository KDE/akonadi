/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "relationfetchjob.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"
#include <QTimer>

using namespace Akonadi;

class Akonadi::RelationFetchJobPrivate : public JobPrivate
{
public:
    explicit RelationFetchJobPrivate(RelationFetchJob *parent)
        : JobPrivate(parent)
    {
        mEmitTimer.setSingleShot(true);
        mEmitTimer.setInterval(std::chrono::milliseconds{100});
    }

    void init()
    {
        QObject::connect(&mEmitTimer, &QTimer::timeout, q_ptr, [this]() { timeout(); });
    }

    void aboutToFinish() override {
        timeout();
    }

    void timeout()
    {
        Q_Q(RelationFetchJob);
        mEmitTimer.stop(); // in case we are called by result()
        if (!mPendingRelations.isEmpty()) {
            if (!q->error()) {
                Q_EMIT q->relationsReceived(mPendingRelations);
            }
            mPendingRelations.clear();
        }
    }

    Q_DECLARE_PUBLIC(RelationFetchJob)

    Relation::List mResultRelations;
    Relation::List mPendingRelations; // relation pending for emitting itemsReceived()
    QTimer mEmitTimer;
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
    if (!d->mEmitTimer.isActive()) {
        d->mEmitTimer.start();
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
