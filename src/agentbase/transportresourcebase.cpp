/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "transportresourcebase.h"
#include "transportresourcebase_p.h"

#include "transportadaptor.h"
#include <QDBusConnection>

#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QDBusConnection>

using namespace Akonadi;

TransportResourceBasePrivate::TransportResourceBasePrivate(TransportResourceBase *qq)
    : q(qq)
{
    new Akonadi__TransportAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Transport"), this, QDBusConnection::ExportAdaptors);
}

void TransportResourceBasePrivate::send(Item::Id id)
{
    auto job = new ItemFetchJob(Item(id));
    job->fetchScope().fetchFullPayload();
    job->setProperty("id", QVariant(id));
    connect(job, &KJob::result, this, &TransportResourceBasePrivate::fetchResult);
}

void TransportResourceBasePrivate::fetchResult(KJob *job)
{
    if (job->error()) {
        const Item::Id id = job->property("id").toLongLong();
        Q_EMIT transportResult(id, static_cast<int>(TransportResourceBase::TransportFailed), job->errorText());
        return;
    }

    auto fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);

    const Item item = fetchJob->items().at(0);
    q->sendItem(item);
}

TransportResourceBase::TransportResourceBase()
    : d(new TransportResourceBasePrivate(this))
{
}

TransportResourceBase::~TransportResourceBase() = default;

void TransportResourceBase::itemSent(const Item &item, TransportResult result, const QString &message)
{
    Q_EMIT d->transportResult(item.id(), static_cast<int>(result), message);
}

#include "moc_transportresourcebase_p.cpp"
