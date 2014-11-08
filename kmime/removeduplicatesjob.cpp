/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Copyright (c) 2010 Andras Mantia <andras@kdab.com>
    Copyright (c) 2012 Dan Vrátil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "removeduplicatesjob.h"

#include <QAbstractItemModel>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchscope.h>
#include <kmime/kmime_message.h>

#include <KLocalizedString>

class Akonadi::RemoveDuplicatesJob::Private {

public:
    Private(RemoveDuplicatesJob *parent)
        : mJobCount(0)
        , mKilled(false)
        , mCurrentJob(0)
        , mParent(parent)
    {
    }

    void fetchItem()
    {
        Akonadi::Collection collection = mFolders.value(mJobCount - 1);
        kDebug() << "Processing collection" << collection.name() << "(" << collection.id() << ")";

        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(collection, mParent);
        job->fetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
        job->fetchScope().fetchFullPayload();
        mParent->connect(job, SIGNAL(result(KJob*)), mParent, SLOT(slotFetchDone(KJob*)));
        mCurrentJob = job;

        emit mParent->description(mParent, i18n("Retrieving items..."));
    }

    void slotFetchDone(KJob *job)
    {
        mJobCount--;
        if (job->error()) {
            mParent->setError(job->error());
            mParent->setErrorText(job->errorText());
            mParent->emitResult();
            return;
        }

        if (mKilled) {
            mParent->emitResult();
            return;
        }

        emit mParent->description(mParent, i18n("Searching for duplicates..."));

        Akonadi::ItemFetchJob *fjob = static_cast<Akonadi::ItemFetchJob *>(job);
        Akonadi::Item::List items = fjob->items();

        //find duplicate mails with the same messageid
        //if duplicates are found, check the content as well to be sure they are the same
        QMap<QByteArray, uint> messageIds;
        QMap<uint, QList<uint> > duplicates;
        QMap<uint, uint> bodyHashes;
        const int numberOfItems(items.size());
        for (int i = 0; i < numberOfItems; ++i) {
            Akonadi::Item item = items.at(i);
            if (item.hasPayload<KMime::Message::Ptr>()) {
                KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();
                QByteArray idStr = message->messageID()->as7BitString(false);
                //TODO: Maybe do some more check in case of idStr.isEmpty()
                //like when the first message's body is different from the 2nd,
                //but the 2nd is the same as the 3rd, etc.
                //if ( !idStr.isEmpty() )
                {
                    if (messageIds.contains(idStr)) {
                        uint mainId = messageIds.value(idStr);
                        if (!bodyHashes.contains(mainId)) {
                            bodyHashes.insert(mainId, qHash(items.value(mainId).payload<KMime::Message::Ptr>()->encodedContent()));
                        }
                        uint hash = qHash(message->encodedContent());
                        kDebug() << idStr << bodyHashes.value(mainId) << hash;
                        if (bodyHashes.value(mainId) == hash) {
                            duplicates[mainId].append(i);
                        }
                    } else {
                        messageIds.insert(idStr, i);
                    }
                }
            }
        }

        QMap<uint, QList<uint> >::ConstIterator end(duplicates.constEnd());
        for (QMap<uint, QList<uint> >::ConstIterator it = duplicates.constBegin(); it != end; ++it) {
            QList<uint>::ConstIterator dupEnd(it.value().constEnd());
            for (QList<uint>::ConstIterator dupIt = it.value().constBegin(); dupIt != dupEnd; ++dupIt) {
                mDuplicateItems.append(items.value(*dupIt));
            }
        }

        if (mKilled) {
            mParent->emitResult();
            return;
        }

        if (mJobCount > 0) {
            fetchItem();
        } else {
            if (mDuplicateItems.isEmpty()) {
                kDebug() << "No duplicates, I'm done here";
                mParent->emitResult();
                return;
            } else {
                emit mParent->description(mParent, i18n("Removing duplicates..."));
                Akonadi::ItemDeleteJob *delCmd = new Akonadi::ItemDeleteJob(mDuplicateItems, mParent);
                mParent->connect(delCmd, SIGNAL(result(KJob*)), mParent, SLOT(slotDeleteDone(KJob*)));
            }
        }
    }

    void slotDeleteDone(KJob *job)
    {
        kDebug() << "Job done";

        mParent->setError(job->error());
        mParent->setErrorText(job->errorText());
        mParent->emitResult();
    }

    Akonadi::Collection::List mFolders;
    int mJobCount;
    Akonadi::Item::List mDuplicateItems;
    bool mKilled;
    Akonadi::Job *mCurrentJob;

private:
    RemoveDuplicatesJob *mParent;

};

using namespace Akonadi;

RemoveDuplicatesJob::RemoveDuplicatesJob(const Akonadi::Collection &folder, QObject *parent)
    : Job(parent)
    , d(new Private(this))
{
    d->mJobCount = 1;   
    d->mFolders << folder;
}

RemoveDuplicatesJob::RemoveDuplicatesJob(const Akonadi::Collection::List &folders, QObject *parent)
    : Job(parent)
    , d(new Private(this))
{
    d->mFolders = folders;
    d->mJobCount = d->mFolders.length();
}

RemoveDuplicatesJob::~RemoveDuplicatesJob()
{
    delete d;
}

void RemoveDuplicatesJob::doStart()
{
    kDebug();

    if (d->mFolders.isEmpty()) {
        kWarning() << "No collections to process";
        emitResult();
        return;
    }

    d->fetchItem();
}

bool RemoveDuplicatesJob::doKill()
{
    kDebug() << "Killed!";

    d->mKilled = true;
    if (d->mCurrentJob) {
        d->mCurrentJob->kill(EmitResult);
    }

    return true;
}

#include "moc_removeduplicatesjob.cpp"
