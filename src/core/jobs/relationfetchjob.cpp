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
#include <QTimer>

using namespace Akonadi;

class Akonadi::RelationFetchJobPrivate : public JobPrivate
{
public:
    RelationFetchJobPrivate(RelationFetchJob *parent)
        : JobPrivate(parent)
        , mEmitTimer(0)
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

    void aboutToFinish()
    {
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
    QTimer *mEmitTimer;
    QStringList mTypes;
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

RelationFetchJob::RelationFetchJob(const QStringList &types, QObject *parent)
    : Job(new RelationFetchJobPrivate(this), parent)
{
    Q_D(RelationFetchJob);
    d->init();
    d->mTypes = types;
}

void RelationFetchJob::doStart()
{
    Q_D(RelationFetchJob);

    QByteArray command = d->newTag();
    command += " UID RELATIONFETCH ";

    QList<QByteArray> filter;
    if (!d->mResource.isEmpty()) {
        filter.append("RESOURCE");
        filter.append(d->mResource.toUtf8());
    }
    if (!d->mTypes.isEmpty()) {
        filter.append("TYPE");
        QList<QByteArray> types;
        foreach (const QString &t, d->mTypes) {
            types.append(t.toUtf8());
        }
        filter.append('(' + ImapParser::join(types, " ") + ')');
    } else if (!d->mRequestedRelation.type().isEmpty()) {
        filter.append("TYPE");
        filter.append('(' + d->mRequestedRelation.type() + ')');
    }
    if (d->mRequestedRelation.left().id() >= 0) {
        filter << "LEFT" << QByteArray::number(d->mRequestedRelation.left().id());
    }
    if (d->mRequestedRelation.right().id() >= 0) {
        filter << "RIGHT" << QByteArray::number(d->mRequestedRelation.right().id());
    }

    command += "(" + ImapParser::join(filter, " ") + ")\n";

    qDebug() << command;
    d->writeData(command);
}

void RelationFetchJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(RelationFetchJob);

    if (tag == "*") {
        int begin = data.indexOf("RELATIONFETCH");
        if (begin >= 0) {
            // split fetch response into key/value pairs
            QList<QByteArray> fetchResponse;
            ImapParser::parseParenthesizedList(data, fetchResponse, begin + 8);

            Relation rel;
            ProtocolHelper::parseRelationFetchResult(fetchResponse, rel);

            if (rel.isValid()) {
                d->mResultRelations.append(rel);
                d->mPendingRelations.append(rel);
                if (!d->mEmitTimer->isActive()) {
                    d->mEmitTimer->start();
                }
            }
            return;
        }
    }
    qDebug() << "Unhandled response: " << tag << data;
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
