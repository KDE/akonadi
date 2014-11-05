/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "transportresourcebase.h"
#include "transportresourcebase_p.h"

#include "KDBusConnectionPool"
#include "transportadaptor.h"

#include "itemfetchjob.h"
#include "itemfetchscope.h"

#include <QtDBus/QDBusConnection>

using namespace Akonadi;

TransportResourceBasePrivate::TransportResourceBasePrivate(TransportResourceBase *qq)
    : QObject()
    , q(qq)
{
    new Akonadi__TransportAdaptor(this);
    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Transport"),
                                                          this, QDBusConnection::ExportAdaptors);
}

void TransportResourceBasePrivate::send(Item::Id id)
{
    ItemFetchJob *job = new ItemFetchJob(Item(id));
    job->fetchScope().fetchFullPayload();
    job->setProperty("id", QVariant(id));
    connect(job, SIGNAL(result(KJob*)), SLOT(fetchResult(KJob*)));
}

void TransportResourceBasePrivate::fetchResult(KJob *job)
{
    if (job->error()) {
        const Item::Id id = job->property("id").toLongLong();
        emit transportResult(id, (int)TransportResourceBase::TransportFailed, job->errorText());
        return;
    }

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);

    const Item item = fetchJob->items().first();
    q->sendItem(item);
}

TransportResourceBase::TransportResourceBase()
    : d(new TransportResourceBasePrivate(this))
{
}

TransportResourceBase::~TransportResourceBase()
{
    delete d;
}

void TransportResourceBase::itemSent(const Item &item,
                                     TransportResult result,
                                     const QString &message)
{
    emit d->transportResult(item.id(), (int)result, message);
}

#include "moc_transportresourcebase_p.cpp"
