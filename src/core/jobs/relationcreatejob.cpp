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

#include "relationcreatejob.h"
#include "job_p.h"
#include "relation.h"
#include "protocolhelper_p.h"
#include <KLocalizedString>

using namespace Akonadi;

struct Akonadi::RelationCreateJobPrivate : public JobPrivate
{
    RelationCreateJobPrivate(RelationCreateJob *parent)
        : JobPrivate(parent)
    {
    }

    Relation mRelation;
};

RelationCreateJob::RelationCreateJob(const Akonadi::Relation &relation, QObject *parent)
    : Job(new RelationCreateJobPrivate(this), parent)
{
    Q_D(RelationCreateJob);
    d->mRelation = relation;
}

void RelationCreateJob::doStart()
{
    Q_D(RelationCreateJob);

    if (!d->mRelation.isValid()) {
        qWarning() << "The relation is invalid";
        setError(Job::Unknown);
        setErrorText(i18n("Failed to create relation."));
        emitResult();
        return;
    }

    QByteArray command = d->newTag() + " UID RELATIONSTORE ";

    QList<QByteArray> list;
    list << "LEFT";
    list << QByteArray::number(d->mRelation.left().id());
    list << "RIGHT";
    list << QByteArray::number(d->mRelation.right().id());
    list << "TYPE";
    list << ImapParser::quote(d->mRelation.type());
    if (!d->mRelation.remoteId().isEmpty()) {
        list << "REMOTEID";
        list << d->mRelation.remoteId();
    }

    command += ImapParser::join(list, " ") + "\n";

    d->writeData(command);
}

void RelationCreateJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(RelationCreateJob);
    qWarning() << "Unhandled response: " << tag << data;
}

Relation RelationCreateJob::relation() const
{
    Q_D(const RelationCreateJob);
    return d->mRelation;
}
