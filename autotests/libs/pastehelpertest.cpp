/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pastehelper_p.h"
#include "interface.h"
#include "shared/akscopeguard.h"

#include <QMimeData>
#include <QUrlQuery>
#include <qtest_akonadi.h>

using namespace Akonadi;

Q_DECLARE_METATYPE(Akonadi::Collection::Right)
Q_DECLARE_METATYPE(Akonadi::Collection::Rights)

class PasteHelperTest : public QObject
{
    Q_OBJECT

    const QString mimeTypeOctetStream = QStringLiteral("application/octet-stream");
    const QString mimeTypeContacts = QStringLiteral("text/directory");

private:
    Collection createCollection(Collection::Rights rights, const QString &mimeType, bool isVirtual)
    {
        Collection col(42);
        col.setRights(rights);
        col.setContentMimeTypes({mimeType});
        col.setVirtual(isVirtual);
        return col;
    }

    QUrl itemUrl(const QString &mimeType)
    {
        QUrl url;
        url.setScheme(QStringLiteral("akonadi"));

        QUrlQuery query;
        query.addQueryItem(QStringLiteral("item"), QStringLiteral("123456"));
        query.addQueryItem(QStringLiteral("type"), mimeType);
        url.setQuery(query);

        return url;
    }

    QUrl collectionUrl()
    {
        QUrl url;
        url.setScheme(QStringLiteral("akonadi"));

        QUrlQuery query;
        query.addQueryItem(QStringLiteral("collection"), QStringLiteral("123"));
        url.setQuery(query);

        return url;
    }

private Q_SLOTS:

    void testCanPaste_data()
    {
        QTest::addColumn<QUrl>("url");
        QTest::addColumn<Collection>("destCollection");
        QTest::addColumn<Qt::DropAction>("dropAction");
        QTest::addColumn<bool>("canPaste");

        QTest::newRow("Copy item into all-rights col")
            << itemUrl(mimeTypeOctetStream)
            << createCollection(Collection::AllRights, mimeTypeOctetStream, false)
            << Qt::CopyAction
            << true;
        QTest::newRow("Move item into all-rights col")
            << itemUrl(mimeTypeOctetStream)
            << createCollection(Collection::AllRights, mimeTypeOctetStream, false)
            << Qt::MoveAction
            << true;
        QTest::newRow("Copy item into read-only col")
            << itemUrl(mimeTypeOctetStream)
            << createCollection(Collection::ReadOnly, mimeTypeOctetStream, false)
            << Qt::CopyAction
            << false;
        QTest::newRow("Move item into read-only col")
            << itemUrl(mimeTypeOctetStream)
            << createCollection(Collection::ReadOnly, mimeTypeOctetStream, false)
            << Qt::MoveAction
            << false;
        QTest::newRow("Copy item into virtual collection")
            << itemUrl(mimeTypeOctetStream)
            << createCollection({Collection::CanLinkItem | Collection::CanUnlinkItem}, mimeTypeOctetStream, true)
            << Qt::CopyAction
            << false;
        QTest::newRow("Move item into virtual collection")
            << itemUrl(mimeTypeOctetStream)
            << createCollection({Collection::CanLinkItem | Collection::CanUnlinkItem}, mimeTypeOctetStream, true)
            << Qt::MoveAction
            << false;
        QTest::newRow("Link into virtual collection")
            << itemUrl(mimeTypeOctetStream)
            << createCollection({Collection::CanLinkItem | Collection::CanUnlinkItem}, mimeTypeOctetStream, true)
            << Qt::LinkAction
            << true;
        QTest::newRow("Copy item into col - mimetype mismatch")
            << itemUrl(mimeTypeOctetStream)
            << createCollection(Collection::AllRights, mimeTypeContacts, false)
            << Qt::CopyAction
            << false;

        QTest::newRow("Copy col into all-rights col")
            << collectionUrl()
            << createCollection(Collection::AllRights, {}, false)
            << Qt::CopyAction
            << true;
        QTest::newRow("Copy col into read-only col")
            << collectionUrl()
            << createCollection(Collection::ReadOnly, {}, false)
            << Qt::CopyAction
            << false;
        QTest::newRow("Move col into all-rights col")
            << collectionUrl()
            << createCollection(Collection::AllRights, {}, false)
            << Qt::MoveAction
            << true;
        QTest::newRow("Move col into read-only col")
            << collectionUrl()
            << createCollection(Collection::ReadOnly, {}, false)
            << Qt::MoveAction
            << false;
    }

    void testCanPaste()
    {
        QFETCH(QUrl, url);
        QFETCH(Collection, destCollection);
        QFETCH(Qt::DropAction, dropAction);
        QFETCH(bool, canPaste);

        QMimeData mimeData;
        mimeData.setUrls({url});
        QCOMPARE(PasteHelper::canPaste(&mimeData, destCollection, dropAction), canPaste);
    }

    void testPasteUriList_data()
    {
        QTest::addColumn<QString>("destMimeType");
        QTest::addColumn<Collection::Rights>("destRights");
        QTest::addColumn<bool>("destIsVirtual");
        QTest::addColumn<Qt::DropAction>("dropAction");
        QTest::addColumn<bool>("success");

        QTest::newRow("Copy into writable col")
            << mimeTypeOctetStream
            << Collection::Rights{Collection::AllRights}
            << false
            << Qt::CopyAction
            << true;

        QTest::newRow("Copy into read-only col")
            << mimeTypeOctetStream
            << Collection::Rights{Collection::ReadOnly}
            << false
            << Qt::CopyAction
            << false;

        QTest::newRow("Move into writable col")
            << mimeTypeOctetStream
            << Collection::Rights{Collection::AllRights}
            << false
            << Qt::MoveAction
            << true;

        QTest::newRow("Link into virtual col")
            << mimeTypeOctetStream
            << Collection::Rights{Collection::CanLinkItem | Collection::CanUnlinkItem}
            << true
            << Qt::LinkAction
            << true;

        QTest::newRow("Move into virtual col")
            << mimeTypeOctetStream
            << Collection::Rights{Collection::CanLinkItem | Collection::CanUnlinkItem}
            << true
            << Qt::MoveAction
            << false;

        QTest::newRow("Copy into virtual col")
            << mimeTypeOctetStream
            << Collection::Rights{Collection::CanLinkItem | Collection::CanUnlinkItem}
            << true
            << Qt::CopyAction
            << false;
    }

    void testPasteUriList()
    {
        QFETCH(QString, destMimeType);
        QFETCH(Collection::Rights, destRights);
        QFETCH(bool, destIsVirtual);
        QFETCH(Qt::DropAction, dropAction);
        QFETCH(bool, success);

        // Create source collection
        const Collection baseCol{AkonadiTest::collectionIdFromPath(QStringLiteral("res1"))};
        Collection sourceCol;
        {
            sourceCol.setParentCollection(baseCol);
            sourceCol.setName(QStringLiteral("pastehelpertest-src"));
            sourceCol.setContentMimeTypes({mimeTypeOctetStream, mimeTypeContacts});
            sourceCol.setRights(Collection::AllRights);
            auto createColTask = Akonadi::createCollection(sourceCol);
            createColTask.wait();
            QVERIFY(!createColTask.hasError());
            sourceCol = createColTask.result();
        }
        AkScopeGuard removeSourceCollection{[sourceCol]() {
            Akonadi::deleteCollection(sourceCol).wait();
        }};

        // Populate source collection with items
        Item item;
        {
            item.setMimeType(mimeTypeOctetStream);
            item.setPayload(QByteArray("FooBar"));
            item.setParentCollection(sourceCol);
            auto task = Akonadi::createItem(item, sourceCol);
            AKVERIFYTASK(task);
            item = task.result();
        }

        // Create destination collection with given properties
        Collection destCol;
        {
            destCol.setParentCollection(baseCol);
            destCol.setName(QStringLiteral("pastehelpertest-dst"));
            destCol.setContentMimeTypes({destMimeType});
            destCol.setRights(destRights);
            destCol.setVirtual(destIsVirtual);
            auto createColTask = Akonadi::createCollection(destCol);
            AKVERIFYTASK(createColTask);
            destCol = createColTask.result();
        }
        AkScopeGuard removeDestcollection{[destCol]() {
            Akonadi::deleteCollection(destCol).wait();
        }};

        // Perform given action
        QMimeData data;
        data.setUrls({item.url(Item::UrlWithMimeType)});

        auto *job = PasteHelper::pasteUriList(&data, destCol, dropAction);
        QCOMPARE((job != nullptr), success);

        if (success) {
            AKVERIFYEXEC(job);

            if (dropAction == Qt::MoveAction) {
                const auto task = Akonadi::fetchItemsFromCollection(sourceCol, ItemFetchOptions{});
                AKVERIFYTASK(task);
                QVERIFY(task.result().empty());
            }

            ItemFetchScope fetchScope;
            fetchScope.fetchFullPayload();
            const auto task = Akonadi::fetchItemsFromCollection(destCol, fetchScope);
            AKVERIFYTASK(task);
            const auto fetchedItems = task.result();

            QCOMPARE(fetchedItems.size(), 1);
            QCOMPARE(fetchedItems[0].payload<QByteArray>(), QByteArray("FooBar"));
        }
    }
};

#include "pastehelpertest.moc"

QTEST_AKONADIMAIN(PasteHelperTest)
