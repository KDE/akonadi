/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QTest>
#include <QEventLoop>

#include "qtest_akonadi.h"
#include "storage.h"

using namespace Akonadi;

class StorageTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCollectionFetch()
    {
        Storage storage;
        QEventLoop eventLoop;
        {
            storage.fetchSubcollections(Collection::root(), CollectionFetchOptions{}, storage.defaultSession())
                .then([&eventLoop](const Collection::List &cols) mutable {
                    QCOMPARE(cols.size(), 4);
                    eventLoop.quit();
                });

        }
        eventLoop.exec();
    }

    void testFetchAllTags()
    {
        Storage storage;
        QEventLoop eventLoop;
        {
            storage.fetchAllTags(TagFetchOptions{}, storage.defaultSession())
                .then([&eventLoop](const Tag::List &tags) mutable {
                    QCOMPARE(tags.size(), 4);
                    eventLoop.quit();
                });
        }
        eventLoop.quit();
    }
};

QTEST_AKONADIMAIN(StorageTest);

#include "storagetest.moc"

