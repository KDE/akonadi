/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemchangelogtest.h"
#include "item_p.h"
#include "itemchangelog_p.h"

#include <QTest>

#include <type_traits>

using namespace Akonadi;

void ItemChangelogTest::testDoesntCrashWhenReassigning()
{
    auto *log = ItemChangeLog::instance();

    // Make the keys non-contiguous so we can trigger rehashing in the next loop
    for (int i = 0; i < 50; i += 2) {
        // We can reinrepret_cast because ItemPrivate pointer is used only
        // as an opaque key in the hash table.
        log->addedFlags(reinterpret_cast<ItemPrivate *>(i)) = {QByteArray("foo")};
    }

    // We will be inserting in between existing items, which will eventually trigger
    // rehashing
    for (int i = 1; i < 50; i += 2) {
        // Previously this would crash because addedFlags() would return a reference
        // to a valud stored in the hash table, which would be invalidated by rehashing
        // caused by the call to left-hand-side addedFlags().
        log->addedFlags(reinterpret_cast<ItemPrivate *>(i)) = log->addedFlags(reinterpret_cast<const ItemPrivate *>(i - 1));
    }
}

QTEST_GUILESS_MAIN(ItemChangelogTest);

#include "moc_itemchangelogtest.cpp"