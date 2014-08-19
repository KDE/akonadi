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

#include "itemfetchjob.h"

#include "attributefactory.h"
#include "collection.h"
#include "collectionselectjob_p.h"
#include <akonadi/private/imapparser_p.h>
#include "itemfetchscope.h"
#include "job_p.h"
#include <akonadi/private/protocol_p.h>
#include "protocolhelper_p.h"
#include "session_p.h"

#include <qdebug.h>

#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
public:
    ItemFetchJobPrivate(ItemFetchJob *parent)
        : JobPrivate(parent)
        , mEmitTimer(0)
        , mValuePool(0)
        , mCount(0)
    {
        mCollection = Collection::root();
        mDeliveryOptions = ItemFetchJob::Default;
    }

    ~ItemFetchJobPrivate()
    {
        delete mValuePool;
    }

    void init()
    {
        Q_Q(ItemFetchJob);
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
        Q_Q(ItemFetchJob);

        mEmitTimer->stop(); // in case we are called by result()
        if (!mPendingItems.isEmpty()) {
            if (!q->error()) {
                emit q->itemsReceived(mPendingItems);
            }
            mPendingItems.clear();
        }
    }

    QString jobDebuggingString() const /*Q_DECL_OVERRIDE*/
    {
        if (mRequestedItems.isEmpty()) {
           QString str = QString::fromLatin1( "All items from collection %1" ).arg( mCollection.id() );
           if ( mFetchScope.fetchChangedSince().isValid() )
             str += QString::fromLatin1( " changed since %1" ).arg( mFetchScope.fetchChangedSince().toString() );
           return str;

        } else {
            try {
                return QString::fromLatin1(ProtocolHelper::entitySetToByteArray(mRequestedItems, AKONADI_CMD_ITEMFETCH));
            } catch (const Exception &e) {
                return QString::fromUtf8(e.what());
            }
        }
    }

    Q_DECLARE_PUBLIC(ItemFetchJob)

    Collection mCollection;
    Tag mTag;
    Item::List mRequestedItems;
    Item::List mResultItems;
    ItemFetchScope mFetchScope;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer *mEmitTimer;
    ProtocolHelperValuePool *mValuePool;
    ItemFetchJob::DeliveryOptions mDeliveryOptions;
    int mCount;
};

ItemFetchJob::ItemFetchJob(const Collection &collection, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);

    d->init();
    d->mCollection = collection;
    d->mValuePool = new ProtocolHelperValuePool; // only worth it for lots of results
}

ItemFetchJob::ItemFetchJob(const Item &item, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);

    d->init();
    d->mRequestedItems.append(item);
}

ItemFetchJob::ItemFetchJob(const Akonadi::Item::List &items, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);

    d->init();
    d->mRequestedItems = items;
}

ItemFetchJob::ItemFetchJob(const QList<Akonadi::Item::Id> &items, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);

    d->init();
    foreach (Item::Id id, items) {
        d->mRequestedItems.append(Item(id));
    }
}

ItemFetchJob::ItemFetchJob(const Tag &tag, QObject *parent)
    : Job(new ItemFetchJobPrivate(this), parent)
{
    Q_D(ItemFetchJob);

    d->init();
    d->mTag = tag;
    d->mValuePool = new ProtocolHelperValuePool;
}

ItemFetchJob::~ItemFetchJob()
{
}

void ItemFetchJob::doStart()
{
    Q_D(ItemFetchJob);

    QByteArray command = d->newTag();
    try {
      command += ProtocolHelper::commandContextToByteArray(d->mCollection, d->mTag, d->mRequestedItems, AKONADI_CMD_ITEMFETCH);
    } catch (const Akonadi::Exception &e) {
      setError(Job::Unknown);
      setErrorText(QString::fromUtf8(e.what()));
      emitResult();
      return;
    }

    // This is only required for 4.10
    if (d->protocolVersion() < 30) {
        if (d->mFetchScope.ignoreRetrievalErrors()) {
            qDebug() << "IGNOREERRORS is not available with this version of Akonadi server";
        }
        d->mFetchScope.setIgnoreRetrievalErrors(false);
    }

    command += ProtocolHelper::itemFetchScopeToByteArray(d->mFetchScope);
    d->writeData(command);
}

void ItemFetchJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(ItemFetchJob);

    if (tag == "*") {
        int begin = data.indexOf("FETCH");
        if (begin >= 0) {

            // split fetch response into key/value pairs
            QList<QByteArray> fetchResponse;
            ImapParser::parseParenthesizedList(data, fetchResponse, begin + 6);

            Item item;
            ProtocolHelper::parseItemFetchResult(fetchResponse, item, d->mValuePool);
            if (!item.isValid()) {
                return;
            }

            d->mCount++;

            if (d->mDeliveryOptions & ItemGetter) {
                d->mResultItems.append(item);
            }

            if (d->mDeliveryOptions & EmitItemsInBatches) {
                d->mPendingItems.append(item);
                if (!d->mEmitTimer->isActive()) {
                    d->mEmitTimer->start();
                }
            } else if (d->mDeliveryOptions & EmitItemsIndividually) {
                emit itemsReceived(Item::List() << item);
            }
            return;
        }
    }
    qDebug() << "Unhandled response: " << tag << data;
}

Item::List ItemFetchJob::items() const
{
    Q_D(const ItemFetchJob);

    return d->mResultItems;
}

void ItemFetchJob::clearItems()
{
    Q_D(ItemFetchJob);

    d->mResultItems.clear();
}

void ItemFetchJob::setFetchScope(const ItemFetchScope &fetchScope)
{
    Q_D(ItemFetchJob);

    d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemFetchJob::fetchScope()
{
    Q_D(ItemFetchJob);

    return d->mFetchScope;
}

void ItemFetchJob::setCollection(const Akonadi::Collection &collection)
{
    Q_D(ItemFetchJob);

    d->mCollection = collection;
}

void ItemFetchJob::setDeliveryOption(DeliveryOptions options)
{
    Q_D(ItemFetchJob);

    d->mDeliveryOptions = options;
}

ItemFetchJob::DeliveryOptions ItemFetchJob::deliveryOptions() const
{
    Q_D(const ItemFetchJob);

    return d->mDeliveryOptions;
}

int ItemFetchJob::count() const
{
    Q_D(const ItemFetchJob);

    return d->mCount;
}
#include "moc_itemfetchjob.cpp"
