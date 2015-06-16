/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collectionselectjob_p.h"

#include "job_p.h"
#include "protocolhelper_p.h"
#include <akonadi/private/protocol_p.h>

using namespace Akonadi;

class Akonadi::CollectionSelectJobPrivate : public JobPrivate
{
public:
    CollectionSelectJobPrivate(CollectionSelectJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const Q_DECL_OVERRIDE { /*Q_DECL_OVERRIDE*/
        if (mCollection.id() > 0) {
            return QString::number(mCollection.id());
        } else {
            return QString::fromLatin1("RemoteID ") + QString::number(mCollection.id());
        }
    }

    Collection mCollection;
};

CollectionSelectJob::CollectionSelectJob(const Collection &collection, QObject *parent)
    : Job(new CollectionSelectJobPrivate(this), parent)
{
    Q_D(CollectionSelectJob);

    d->mCollection = collection;
}

CollectionSelectJob::~CollectionSelectJob()
{
}

void CollectionSelectJob::doStart()
{
    Q_D(CollectionSelectJob);

    try {
        d->sendCommand(ProtocolHelper::entityToScope(d->mCollection));
    } catch (const std::exception &e) {
        setError(Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
    }
}

void CollectionSelectJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    if (!response.isResponse() || response.type() != Protocol::Command::SelectCollection) {
        Job::doHandleResponse(tag, response);
        return;
    }

    emitResult();
}

#include "moc_collectionselectjob_p.cpp"
