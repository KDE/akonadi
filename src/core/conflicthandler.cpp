/*
    SPDX-FileCopyrightText: 2010 KDAB
    SPDX-FileContributor: Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "conflicthandler_p.h"
#include "itemfetchscope.h"
#include "interface.h"
#include "session.h"
#include <KLocalizedString>

using namespace Akonadi;

ConflictHandler::ConflictHandler(ConflictType type, QObject *parent)
    : QObject(parent)
    , mConflictType(type)
    , mSession(new Session("conflict handling session", this))
{
}

void ConflictHandler::setConflictingItems(const Akonadi::Item &changedItem, const Akonadi::Item &conflictingItem)
{
    mChangedItem = changedItem;
    mConflictingItem = conflictingItem;
}

void ConflictHandler::start()
{
    if (mConflictType == LocalLocalConflict || mConflictType == LocalRemoteConflict) {
        ItemFetchOptions options;
        options.itemFetchScope().fetchFullPayload();
        options.itemFetchScope().setAncestorRetrieval(ItemFetchScope::Parent);
        Akonadi::fetchItem(mConflictingItem, options, mSession)
            .then([this](const Item &item) {
                      if (!item.isValid()) {
                          Q_EMIT error(i18n("Did not find other item for conflict handling."));
                          return;
                      }

                      mConflictingItem = item;
                      QMetaObject::invokeMethod(this, &ConflictHandler::resolve, Qt::QueuedConnection);
                  },
                  [this](const Error &error) {
                      Q_EMIT this->error(error.message());
                  });
    } else {
        resolve();
    }
}

void ConflictHandler::resolve()
{
#pragma message ("warning KF5 Port me!")
#if 0
    ConflictResolveDialog dlg;
    dlg.setConflictingItems(mChangedItem, mConflictingItem);
    dlg.exec();

    const ResolveStrategy strategy = dlg.resolveStrategy();
    switch (strategy) {
    case UseLocalItem:
        useLocalItem();
        break;
    case UseOtherItem:
        useOtherItem();
        break;
    case UseBothItems:
        useBothItems();
        break;
    }
#endif
}

void ConflictHandler::useLocalItem()
{
    // We have to overwrite the other item inside the Akonadi storage with the local
    // item. To make this happen, we have to set the revision of the local item to
    // the one of the other item to let the Akonadi server accept it.

    Item newItem(mChangedItem);
    newItem.setRevision(mConflictingItem.revision());
    Akonadi::updateItem(newItem, ItemModifyFlag::Default, mSession)
        .then([this](const Item &/*item*/) {
                  Q_EMIT conflictResolved();
              },
              [this](const Error &error) {
                  Q_EMIT this->error(error.message());
              });
}

void ConflictHandler::useOtherItem()
{
    // We can just ignore the local item here and leave everything as it is.
    Q_EMIT conflictResolved();
}

void ConflictHandler::useBothItems()
{
    // We have to create a new item for the local item under the collection that has
    // been retrieved when we fetched the other item.
    Akonadi::createItem(mChangedItem, mConflictingItem.parentCollection(), ItemCreateFlag::Default, mSession)
        .then([this](const Item & /*item*/) {
                  Q_EMIT conflictResolved();
              },
              [this](const Error &error) {
                  Q_EMIT this->error(error.message());
              });
}
