/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "pastehelper_p.h"

#include "collectioncopyjob.h"
#include "collectionmovejob.h"
#include "item.h"
#include "itemcreatejob.h"
#include "itemcopyjob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "linkjob.h"
#include "transactionsequence.h"
#include "session.h"

#include <KUrl>

#include <QtCore/QByteArray>
#include <QtCore/QMimeData>
#include <QtCore/QStringList>

using namespace Akonadi;

bool PasteHelper::canPaste(const QMimeData *mimeData, const Collection &collection)
{
    if (!mimeData || !collection.isValid()) {
        return false;
    }

    // check that the target collection has the rights to
    // create the pasted items resp. collections
    Collection::Rights neededRights = Collection::ReadOnly;
    if (KUrl::List::canDecode(mimeData)) {
        const KUrl::List urls = KUrl::List::fromMimeData(mimeData);
        foreach (const KUrl &url, urls) {
            if (url.hasQueryItem(QStringLiteral("item"))) {
                neededRights |= Collection::CanCreateItem;
            } else if (url.hasQueryItem(QStringLiteral("collection"))) {
                neededRights |= Collection::CanCreateCollection;
            }
        }

        if ((collection.rights() & neededRights) == 0) {
            return false;
        }

        // check that the target collection supports the mime types of the
        // items/collections that shall be pasted
        bool supportsMimeTypes = true;
        foreach (const KUrl &url, urls) {
            // collections do not provide mimetype information, so ignore this check
            if (url.hasQueryItem(QStringLiteral("collection"))) {
                continue;
            }

            const QString mimeType = url.queryItemValue(QStringLiteral("type"));
            if (!collection.contentMimeTypes().contains(mimeType)) {
                supportsMimeTypes = false;
                break;
            }
        }

        if (!supportsMimeTypes) {
            return false;
        }

        return true;
    }

    return false;
}

KJob *PasteHelper::paste(const QMimeData *mimeData, const Collection &collection, bool copy, Session *session)
{
    if (!canPaste(mimeData, collection)) {
        return 0;
    }

    // we try to drop data not coming with the akonadi:// url
    // find a type the target collection supports
    foreach (const QString &type, mimeData->formats()) {
        if (!collection.contentMimeTypes().contains(type)) {
            continue;
        }

        QByteArray item = mimeData->data(type);
        // HACK for some unknown reason the data is sometimes 0-terminated...
        if (!item.isEmpty() && item.at(item.size() - 1) == 0) {
            item.resize(item.size() - 1);
        }

        Item it;
        it.setMimeType(type);
        it.setPayloadFromData(item);

        ItemCreateJob *job = new ItemCreateJob(it, collection);
        return job;
    }

    if (!KUrl::List::canDecode(mimeData)) {
        return 0;
    }

    // data contains an url list
    return pasteUriList(mimeData, collection, copy ? Qt::CopyAction : Qt::MoveAction, session);
}

KJob *PasteHelper::pasteUriList(const QMimeData *mimeData, const Collection &destination, Qt::DropAction action, Session *session)
{
    if (!KUrl::List::canDecode(mimeData)) {
        return 0;
    }

    if (!canPaste(mimeData, destination)) {
        return 0;
    }

    const KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    Collection::List collections;
    Item::List items;
    foreach (const KUrl &url, urls) {
        const Collection collection = Collection::fromUrl(url);
        if (collection.isValid()) {
            collections.append(collection);
        }
        const Item item = Item::fromUrl(url);
        if (item.isValid()) {
            items.append(item);
        }
        // TODO: handle non Akonadi URLs?
    }

    TransactionSequence *transaction = new TransactionSequence(session);

    //FIXME: The below code disables transactions in otder to avoid data loss due to nested
    //transactions (copy and colcopy in the server doesn't see the items retrieved into the cache and copies empty payloads).
    //Remove once this is fixed properly, see the other FIXME comments.
    transaction->setProperty("transactionsDisabled", true);

    switch (action) {
    case Qt::CopyAction:
        if (!items.isEmpty()) {
            new ItemCopyJob(items, destination, transaction);
        }
        foreach (const Collection &col, collections) {   // FIXME: remove once we have a batch job for collections as well
            new CollectionCopyJob(col, destination, transaction);
        }
        break;
    case Qt::MoveAction:
        if (!items.isEmpty()) {
            new ItemMoveJob(items, destination, transaction);
        }
        foreach (const Collection &col, collections) {   // FIXME: remove once we have a batch job for collections as well
            new CollectionMoveJob(col, destination, transaction);
        }
        break;
    case Qt::LinkAction:
        new LinkJob(destination, items, transaction);
        break;
    default:
        Q_ASSERT(false); // WTF?!
        return 0;
    }
    return transaction;
}
