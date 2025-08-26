/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemcopyjob.h"

#include "collection.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

using namespace Akonadi;

class Akonadi::ItemCopyJobPrivate : public JobPrivate
{
public:
    explicit ItemCopyJobPrivate(ItemCopyJob *parent)
        : JobPrivate(parent)
    {
    }
    QString jobDebuggingString() const override;

    Item::List mItems;
    Collection mTarget;
};

QString Akonadi::ItemCopyJobPrivate::jobDebuggingString() const
{
    QString str = QStringLiteral("Copy items : ");
    const int nbItems = mItems.count();
    for (int i = 0; i < nbItems; ++i) {
        if (i != 0) {
            str += u',';
        }
        str += QString::number(mItems.at(i).id());
    }
    return str + QStringLiteral(" to collection %1").arg(mTarget.id());
}

ItemCopyJob::ItemCopyJob(const Item &item, const Collection &target, QObject *parent)
    : Job(new ItemCopyJobPrivate(this), parent)
{
    Q_D(ItemCopyJob);

    d->mItems << item;
    d->mTarget = target;
}

ItemCopyJob::ItemCopyJob(const Item::List &items, const Collection &target, QObject *parent)
    : Job(new ItemCopyJobPrivate(this), parent)
{
    Q_D(ItemCopyJob);

    d->mItems = items;
    d->mTarget = target;
}

ItemCopyJob::~ItemCopyJob()
{
}

void ItemCopyJob::doStart()
{
    Q_D(ItemCopyJob);

    try {
        d->sendCommand(Protocol::CopyItemsCommandPtr::create(ProtocolHelper::entitySetToScope(d->mItems), ProtocolHelper::entityToScope(d->mTarget)));
    } catch (std::exception &e) {
        setError(Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
    }
}

bool ItemCopyJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::CopyItems) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

#include "moc_itemcopyjob.cpp"
