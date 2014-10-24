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

#include "itemmodifyjob.h"
#include "itemmodifyjob_p.h"

#include "changemediator_p.h"
#include "collection.h"
#include "conflicthandling/conflicthandler_p.h"
#include "entity_p.h"
#include "imapparser_p.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "gid/gidextractor_p.h"

#include <kdebug.h>

using namespace Akonadi;

ItemModifyJobPrivate::ItemModifyJobPrivate(ItemModifyJob *parent)
    : JobPrivate(parent)
    , mRevCheck(true)
    , mIgnorePayload(false)
    , mAutomaticConflictHandlingEnabled(true)
    , mSilent(false)
{
}

void ItemModifyJobPrivate::setClean()
{
    mOperations.insert(Dirty);
}

QByteArray ItemModifyJobPrivate::nextPartHeader()
{
    QByteArray command;
    if (!mParts.isEmpty()) {
        QSetIterator<QByteArray> it(mParts);
        const QByteArray label = it.next();
        mParts.remove(label);

        mPendingData.clear();
        int version = 0;
        ItemSerializer::serialize(mItems.first(), label, mPendingData, version);
        command += ' ' + ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartPayload, label, version);
        if (mPendingData.size() > 0) {
            command += " {" + QByteArray::number(mPendingData.size()) + "}\n";
        } else {
            if (mPendingData.isNull()) {
                command += " NIL";
            } else {
                command += " \"\"";
            }
            command += nextPartHeader();
        }
    } else {
        command += ")\n";
    }
    return command;
}

void ItemModifyJobPrivate::conflictResolved()
{
    Q_Q(ItemModifyJob);

    q->setError(KJob::NoError);
    q->setErrorText(QString());
    q->emitResult();
}

void ItemModifyJobPrivate::conflictResolveError(const QString &message)
{
    Q_Q(ItemModifyJob);

    q->setErrorText(q->errorText() + message);
    q->emitResult();
}

void ItemModifyJobPrivate::doUpdateItemRevision(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    Item::List::iterator it = std::find_if(mItems.begin(), mItems.end(), boost::bind(&Item::id, _1) == itemId);
    if (it != mItems.end() && (*it).revision() == oldRevision) {
        (*it).setRevision(newRevision);
    }
}

QString ItemModifyJobPrivate::jobDebuggingString() const
{
    try {
        return QString::fromUtf8(fullCommand());
    } catch (const Exception &e) {
        return QString::fromUtf8(e.what());
    }
}

void ItemModifyJobPrivate::setSilent( bool silent )
{
  mSilent = silent;
}

QByteArray ItemModifyJobPrivate::tagsToCommandParameter(const Tag::List &tags) const
{
    QByteArray c;
    if (tags.isEmpty() || (tags.first().id() >= 0)) {
        c += "TAGS";
        if (tags.isEmpty()) {
            c += " ()";
        } else {
            c += ' ' + ProtocolHelper::tagSetToImapSequenceSet(tags);
        }
    } else if (std::find_if(tags.constBegin(), tags.constEnd(),
                        boost::bind(&QByteArray::isEmpty, boost::bind(&Tag::remoteId, _1)))
        == tags.constEnd()) {
        //All items have a remoteId
        c += "RTAGS";
        QList<QByteArray> rids;
        Q_FOREACH (const Tag &object, tags) {
            rids << ImapParser::quote(object.remoteId());
        }

        c += '(';
        c += ImapParser::join(rids, " ");
        c += ')';

    } else if (std::find_if(tags.constBegin(), tags.constEnd(),
                        boost::bind(&QByteArray::isEmpty, boost::bind(&Tag::gid, _1)))
        == tags.constEnd()) {
        //All items have a gid
        c += "GTAGS";
        QList<QByteArray> gids;
        Q_FOREACH (const Tag &object, tags) {
            gids << ImapParser::quote(object.gid());
        }

        c += '(';
        c += ImapParser::join(gids, " ");
        c += ')';
    } else {
        throw Exception("Cannot identify all tags");
    }
    return c;
}

ItemModifyJob::ItemModifyJob(const Item &item, QObject *parent)
    : Job(new ItemModifyJobPrivate(this), parent)
{
    Q_D(ItemModifyJob);

    d->mItems.append(item);
    d->mParts = item.loadedPayloadParts();

    d->mOperations.insert(ItemModifyJobPrivate::RemoteId);
    d->mOperations.insert(ItemModifyJobPrivate::RemoteRevision);
}

ItemModifyJob::ItemModifyJob(const Akonadi::Item::List &items, QObject *parent)
    : Job(new ItemModifyJobPrivate(this), parent)
{
    Q_ASSERT(!items.isEmpty());
    Q_D(ItemModifyJob);
    d->mItems = items;

    // same as single item ctor
    if (d->mItems.size() == 1) {
        d->mParts = items.first().loadedPayloadParts();
        d->mOperations.insert(ItemModifyJobPrivate::RemoteId);
        d->mOperations.insert(ItemModifyJobPrivate::RemoteRevision);
    } else {
        d->mIgnorePayload = true;
        d->mRevCheck = false;
    }
}

ItemModifyJob::~ItemModifyJob()
{
}

QByteArray ItemModifyJobPrivate::fullCommand() const
{
    const Akonadi::Item item = mItems.first();
    QList<QByteArray> changes;
    foreach (int op, mOperations) {
        switch (op) {
        case ItemModifyJobPrivate::RemoteId:
            if (!item.remoteId().isNull()) {
                changes << "REMOTEID";
                changes << ImapParser::quote(item.remoteId().toUtf8());
            }
            break;
        case ItemModifyJobPrivate::Gid: {
            const QString gid = GidExtractor::getGid(item);
            if (!gid.isNull()) {
                changes << "GID";
                changes << ImapParser::quote(gid.toUtf8());
            }
            break;
        }
        case ItemModifyJobPrivate::RemoteRevision:
            if (!item.remoteRevision().isNull()) {
                changes << "REMOTEREVISION";
                changes << ImapParser::quote(item.remoteRevision().toUtf8());
            }
            break;
        case ItemModifyJobPrivate::Dirty:
            changes << "DIRTY";
            changes << "false";
            break;
        }
    }

    if (item.d_func()->mClearPayload) {
        changes << "INVALIDATECACHE";
    }
  if ( mSilent ) {
    changes << "SILENT";
  }

    if (item.d_func()->mFlagsOverwritten) {
        changes << "FLAGS";
        changes << '(' + ImapParser::join(item.flags(), " ") + ')';
    } else {
        if (!item.d_func()->mAddedFlags.isEmpty()) {
            changes << "+FLAGS";
            changes << '(' + ImapParser::join(item.d_func()->mAddedFlags, " ") + ')';
        }
        if (!item.d_func()->mDeletedFlags.isEmpty()) {
            changes << "-FLAGS";
            changes << '(' + ImapParser::join(item.d_func()->mDeletedFlags, " ") + ')';
        }
    }

    if (item.d_func()->mTagsOverwritten) {
        changes << tagsToCommandParameter(item.tags());
    } else {
        if (!item.d_func()->mAddedTags.isEmpty()) {
            changes << "+" + tagsToCommandParameter(item.d_func()->mAddedTags);
        }
        if (!item.d_func()->mDeletedTags.isEmpty()) {
            changes << "-" + tagsToCommandParameter(item.d_func()->mDeletedTags);
        }
    }

    if (!item.d_func()->mDeletedAttributes.isEmpty()) {
        changes << "-PARTS";
        QList<QByteArray> attrs;
        foreach (const QByteArray &attr, item.d_func()->mDeletedAttributes) {
            attrs << ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartAttribute, attr);
        }
        changes << '(' + ImapParser::join(attrs, " ") + ')';
    }

    // nothing to do
    if (changes.isEmpty() && mParts.isEmpty() && item.attributes().isEmpty()) {
        return QByteArray();
    }

    QByteArray command;
    command += ProtocolHelper::entitySetToByteArray(mItems, "STORE");   // can throw an exception
    command += ' ';
    if (!mRevCheck || item.revision() < 0) {
        command += "NOREV ";
    } else {
        command += "REV " + QByteArray::number(item.revision()) + ' ';
    }

    if (item.d_func()->mSizeChanged) {
        command += "SIZE " + QByteArray::number(item.size());
    }

    command += " (" + ImapParser::join(changes, " ");
    const QByteArray attrs = ProtocolHelper::attributesToByteArray(item, true);
    if (!attrs.isEmpty()) {
        command += ' ' + attrs;
    }
    return command;
}

void ItemModifyJob::doStart()
{
    Q_D(ItemModifyJob);

    QByteArray command;
    try {
        command = d->fullCommand();
    } catch (const Exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }
    if (command.isEmpty()) {
        emitResult();
        return;
    }

    d->mTag = d->newTag();
    command.prepend(d->mTag);

    command += d->nextPartHeader();

    d->writeData(command);
    d->newTag(); // hack to circumvent automatic response handling
}

void ItemModifyJob::doHandleResponse(const QByteArray &_tag, const QByteArray &data)
{
    Q_D(ItemModifyJob);

    if (_tag == "+") {   // ready for literal data
        if (data.startsWith("STREAM")) {
            QByteArray error;
            if (!ProtocolHelper::streamPayloadToFile(data, d->mPendingData, error)) {
                d->writeData("* NO " + error);
                return;
            }
        } else {
            d->writeData(d->mPendingData);
        }
        d->writeData(d->nextPartHeader());
        return;
    }

    if (_tag == d->mTag) {
        if (data.startsWith("OK")) {     //krazy:exclude=strings
            QDateTime modificationDateTime;
            int dateTimePos = data.indexOf("DATETIME");
            if (dateTimePos != -1) {
                int resultPos = ImapParser::parseDateTime(data, modificationDateTime, dateTimePos + 8);
                if (resultPos == (dateTimePos + 8)) {
                    kDebug() << "Invalid DATETIME response to STORE command: " << _tag << data;
                }
            }

            Item &item = d->mItems.first();
            item.setModificationTime(modificationDateTime);
            item.d_ptr->resetChangeLog();
        } else {
            setError(Unknown);
            setErrorText(QString::fromUtf8(data));

            if (data.contains("[LLCONFLICT]")) {
                if (d->mAutomaticConflictHandlingEnabled) {
                    ConflictHandler *handler = new ConflictHandler(ConflictHandler::LocalLocalConflict, this);
                    handler->setConflictingItems(d->mItems.first(), d->mItems.first());
                    connect(handler, SIGNAL(conflictResolved()), SLOT(conflictResolved()));
                    connect(handler, SIGNAL(error(QString)), SLOT(conflictResolveError(QString)));

                    QMetaObject::invokeMethod(handler, "start", Qt::QueuedConnection);
                    return;
                }
            }
        }

        foreach (const Item &item, d->mItems) {
            ChangeMediator::invalidateItem(item);
        }

        emitResult();
        return;
    }

    if (_tag == "*") {
        Akonadi::Item::Id id;
        ImapParser::parseNumber(data, id);
        int pos = data.indexOf('(');
        if (pos <= 0 || id <= 0) {
            kDebug() << "Ignoring strange response: " << _tag << data;
            return;
        }
        Item::List::iterator it = std::find_if(d->mItems.begin(), d->mItems.end(), boost::bind(&Item::id, _1) == id);
        if (it == d->mItems.end()) {
            kDebug() << "Received STORE response for an item we did not modify: " << _tag << data;
            return;
        }
        QList<QByteArray> attrs;
        ImapParser::parseParenthesizedList(data, attrs, pos);
        for (int i = 0; i < attrs.size() - 1; i += 2) {
            const QByteArray key = attrs.at(i);
            if (key == "REV") {
                const int newRev = attrs.at(i + 1).toInt();
                const int oldRev = (*it).revision();
                if (newRev < oldRev || newRev < 0) {
                    continue;
                }
                d->itemRevisionChanged((*it).id(), oldRev, newRev);
                (*it).setRevision(newRev);
            }
        }
        return;
    }

    kDebug() << "Unhandled response: " << _tag << data;
}

void ItemModifyJob::setIgnorePayload(bool ignore)
{
    Q_D(ItemModifyJob);

    if (d->mIgnorePayload == ignore) {
        return;
    }

    d->mIgnorePayload = ignore;
    if (d->mIgnorePayload) {
        d->mParts = QSet<QByteArray>();
    } else {
        Q_ASSERT(!d->mItems.first().mimeType().isEmpty());
        d->mParts = d->mItems.first().loadedPayloadParts();
    }
}

bool ItemModifyJob::ignorePayload() const
{
    Q_D(const ItemModifyJob);

    return d->mIgnorePayload;
}

void ItemModifyJob::setUpdateGid(bool update)
{
    Q_D(ItemModifyJob);
    if (update && !updateGid()) {
        d->mOperations.insert(ItemModifyJobPrivate::Gid);
    } else {
        d->mOperations.remove(ItemModifyJobPrivate::Gid);
    }
}

bool ItemModifyJob::updateGid() const
{
    Q_D(const ItemModifyJob);
    return d->mOperations.contains(ItemModifyJobPrivate::Gid);
}

void ItemModifyJob::disableRevisionCheck()
{
    Q_D(ItemModifyJob);

    d->mRevCheck = false;
}

void ItemModifyJob::disableAutomaticConflictHandling()
{
    Q_D(ItemModifyJob);

    d->mAutomaticConflictHandlingEnabled = false;
}

Item ItemModifyJob::item() const
{
    Q_D(const ItemModifyJob);
    Q_ASSERT(d->mItems.size() == 1);

    return d->mItems.first();
}

Item::List ItemModifyJob::items() const
{
    Q_D(const ItemModifyJob);
    return d->mItems;
}

#include "moc_itemmodifyjob.cpp"
