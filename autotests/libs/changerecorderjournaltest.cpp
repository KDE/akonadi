/*
    SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changerecorderjournal_p.h"
#include "private/protocol_p.h"

#include <QFile>
#include <QQueue>
#include <QTemporaryDir>
#include <QTest>

using namespace Akonadi;

class ChangeRecorderJournalTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shouldRoundTripTagNotification()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString fileName = dir.filePath(QStringLiteral("changes.dat"));

        Protocol::FetchTagsResponse tag;
        tag.setId(42);
        tag.setParentId(7);
        tag.setGid("tag-gid");
        tag.setType("PLAIN");
        tag.setRemoteId("tag-rid");
        tag.setAttributes({{"attr1", "value1"}, {"attr2", "value2"}});

        auto tagMsg = Protocol::TagChangeNotificationPtr::create();
        tagMsg->setOperation(Protocol::TagChangeNotification::Modify);
        tagMsg->setResource("akonadi_test_resource_0");
        tagMsg->setTag(tag);

        // An item notification after the tag one checks that the stream stays in sync
        Protocol::FetchItemsResponse item;
        item.setId(1234);
        item.setRemoteId(QStringLiteral("item-rid"));
        item.setMimeType(QStringLiteral("text/plain"));

        auto itemMsg = Protocol::ItemChangeNotificationPtr::create();
        itemMsg->setOperation(Protocol::ItemChangeNotification::Add);
        itemMsg->setResource("akonadi_test_resource_0");
        itemMsg->setParentCollection(5);
        itemMsg->setItems({item});

        QQueue<Protocol::ChangeNotificationPtr> changes;
        changes.enqueue(tagMsg);
        changes.enqueue(itemMsg);

        {
            QFile file(fileName);
            QVERIFY(file.open(QIODevice::WriteOnly));
            ChangeRecorderJournalWriter::saveTo(changes, &file);
        }

        QFile file(fileName);
        QVERIFY(file.open(QIODevice::ReadOnly));
        bool needsFullSave = true;
        const auto loaded = ChangeRecorderJournalReader::loadFrom(&file, needsFullSave);
        QCOMPARE(loaded.count(), 2);
        QVERIFY(!needsFullSave);

        QCOMPARE(loaded.at(0)->type(), Protocol::Command::TagChangeNotification);
        const auto loadedTagMsg = Protocol::cmdCast<Protocol::TagChangeNotification>(loaded.at(0));
        QCOMPARE(loadedTagMsg.operation(), Protocol::TagChangeNotification::Modify);
        QCOMPARE(loadedTagMsg.resource(), QByteArray("akonadi_test_resource_0"));
        const auto &loadedTag = loadedTagMsg.tag();
        QCOMPARE(loadedTag.id(), tag.id());
        QCOMPARE(loadedTag.parentId(), tag.parentId());
        QCOMPARE(loadedTag.gid(), tag.gid());
        QCOMPARE(loadedTag.type(), tag.type());
        QCOMPARE(loadedTag.remoteId(), tag.remoteId());
        QCOMPARE(loadedTag.attributes(), tag.attributes());

        QCOMPARE(loaded.at(1)->type(), Protocol::Command::ItemChangeNotification);
        const auto loadedItemMsg = Protocol::cmdCast<Protocol::ItemChangeNotification>(loaded.at(1));
        QCOMPARE(loadedItemMsg.operation(), Protocol::ItemChangeNotification::Add);
        QCOMPARE(loadedItemMsg.parentCollection(), 5);
        QCOMPARE(loadedItemMsg.items().count(), 1);
        QCOMPARE(loadedItemMsg.items().at(0).id(), 1234);
        QCOMPARE(loadedItemMsg.items().at(0).remoteId(), QStringLiteral("item-rid"));
    }
};

QTEST_GUILESS_MAIN(ChangeRecorderJournalTest)

#include "changerecorderjournaltest.moc"
