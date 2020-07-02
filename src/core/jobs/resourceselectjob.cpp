/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resourceselectjob_p.h"

#include "job_p.h"
#include "private/imapparser_p.h"
#include "private/protocol_p.h"

using namespace Akonadi;

class Akonadi::ResourceSelectJobPrivate : public JobPrivate
{
public:
    explicit ResourceSelectJobPrivate(ResourceSelectJob *parent)
        : JobPrivate(parent)
    {
    }

    QString resourceId;
    QString jobDebuggingString() const override;
};

QString Akonadi::ResourceSelectJobPrivate::jobDebuggingString() const
{
    return QStringLiteral("Select Resource %1").arg(resourceId);
}

ResourceSelectJob::ResourceSelectJob(const QString &identifier, QObject *parent)
    : Job(new ResourceSelectJobPrivate(this), parent)
{
    Q_D(ResourceSelectJob);
    d->resourceId = identifier;
}

void ResourceSelectJob::doStart()
{
    Q_D(ResourceSelectJob);

    d->sendCommand(Protocol::SelectResourceCommandPtr::create(d->resourceId));
}

bool ResourceSelectJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::SelectResource) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

#include "moc_resourceselectjob_p.cpp"
