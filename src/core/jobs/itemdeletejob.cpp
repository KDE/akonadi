/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemdeletejob.h"

#include "collection.h"
#include "item.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "private/protocol_p.h"


using namespace Akonadi;

class Akonadi::ItemDeleteJobPrivate : public JobPrivate
{
public:
    explicit ItemDeleteJobPrivate(ItemDeleteJob *parent)
        : JobPrivate(parent)
    {
    }

    Q_DECLARE_PUBLIC(ItemDeleteJob)
    QString jobDebuggingString() const override;

    Item::List mItems;
    Collection mCollection;
    Tag mCurrentTag;

};

QString Akonadi::ItemDeleteJobPrivate::jobDebuggingString() const
{
    QString itemStr = QStringLiteral("items id: ");
    bool firstItem = true;
    for (const Akonadi::Item &item : qAsConst(mItems)) {
        if (firstItem) {
            firstItem = false;
        } else {
            itemStr += QStringLiteral(", ");
        }
        itemStr += QString::number(item.id());
    }

    return QStringLiteral("Remove %1 from collection id %2").arg(itemStr).arg(mCollection.id());
}

ItemDeleteJob::ItemDeleteJob(const Item &item, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mItems << item;
}

ItemDeleteJob::ItemDeleteJob(const Item::List &items, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mItems = items;
}

ItemDeleteJob::ItemDeleteJob(const Collection &collection, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mCollection = collection;
}

ItemDeleteJob::ItemDeleteJob(const Tag &tag, QObject *parent)
    : Job(new ItemDeleteJobPrivate(this), parent)
{
    Q_D(ItemDeleteJob);

    d->mCurrentTag = tag;
}

ItemDeleteJob::~ItemDeleteJob()
{
}

Item::List ItemDeleteJob::deletedItems() const
{
    Q_D(const ItemDeleteJob);

    return d->mItems;
}

void ItemDeleteJob::doStart()
{
    Q_D(ItemDeleteJob);

    try {
        d->sendCommand(Protocol::DeleteItemsCommandPtr::create(
                           d->mItems.isEmpty() ? Scope() : ProtocolHelper::entitySetToScope(d->mItems),
                           ProtocolHelper::commandContextToProtocol(d->mCollection, d->mCurrentTag, d->mItems)));
    } catch (const Akonadi::Exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }
}

bool ItemDeleteJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    if (!response->isResponse() || response->type() != Protocol::Command::DeleteItems) {
        return Job::doHandleResponse(tag, response);
    }

    return true;
}

#include "moc_itemdeletejob.cpp"
