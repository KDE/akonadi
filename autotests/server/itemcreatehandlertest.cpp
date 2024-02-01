/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QSettings>

#include <qhashfunctions.h>
#include <storage/selectquerybuilder.h>

#include "private/scope_p.h"
#include "private/standarddirs_p.h"

#include "fakeakonadiserver.h"
#include "fakeentities.h"

#include "shared/akranges.h"
#include "shared/aktest.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(PimItem)
Q_DECLARE_METATYPE(QList<Flag>)
Q_DECLARE_METATYPE(QList<FakePart>)
Q_DECLARE_METATYPE(QList<FakeTag>)

class ItemCreateHandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;

public:
    ItemCreateHandlerTest()
    {
        QHashSeed::setDeterministicGlobalSeed();

        // Effectively disable external payload parts, we have a dedicated unit-test
        // for that
        const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
        QSettings settings(serverConfigFile, QSettings::IniFormat);
        settings.setValue(QStringLiteral("General/SizeThreshold"), std::numeric_limits<qint64>::max());

        mAkonadi.init();
    }

    void updatePimItem(PimItem &pimItem, const QString &remoteId, const qint64 size)
    {
        pimItem.setRemoteId(remoteId);
        pimItem.setGid(remoteId);
        pimItem.setSize(size);
    }

    void updateNotifcationEntity(Protocol::ItemChangeNotificationPtr &ntf, const PimItem &pimItem)
    {
        Protocol::FetchItemsResponse item;
        item.setId(pimItem.id());
        item.setRemoteId(pimItem.remoteId());
        item.setRemoteRevision(pimItem.remoteRevision());
        item.setMimeType(pimItem.mimeType().name());
        ntf->setItems({std::move(item)});
    }

    struct PartHelper {
        PartHelper(const QString &type_, const QByteArray &data_, qsizetype size_, Part::Storage storage_ = Part::Internal, int version_ = 0)
            : type(type_)
            , data(data_)
            , size(size_)
            , storage(storage_)
            , version(version_)
        {
        }
        QString type;
        QByteArray data;
        qsizetype size;
        Part::Storage storage;
        int version;
    };

    void updateParts(QList<FakePart> &parts, const std::vector<PartHelper> &updatedParts)
    {
        parts.clear();
        for (const PartHelper &helper : updatedParts) {
            FakePart part;

            const QStringList types = helper.type.split(QLatin1Char(':'));
            Q_ASSERT(types.count() == 2);
            part.setPartType(PartType(types[1], types[0]));
            part.setData(helper.data);
            part.setDatasize(helper.size);
            part.setStorage(helper.storage);
            part.setVersion(helper.version);
            parts << part;
        }
    }

    void updateFlags(QList<Flag> &flags, const QStringList &updatedFlags)
    {
        flags.clear();
        for (const QString &flagName : updatedFlags) {
            Flag flag;
            flag.setName(flagName);
            flags << flag;
        }
    }

    struct TagHelper {
        TagHelper(const QString &tagType_, const QString &gid_, const QString &remoteId_ = QString())
            : tagType(tagType_)
            , gid(gid_)
            , remoteId(remoteId_)
        {
        }
        QString tagType;
        QString gid;
        QString remoteId;
    };
    void updateTags(QList<FakeTag> &tags, const std::vector<TagHelper> &updatedTags)
    {
        tags.clear();
        for (const TagHelper &helper : updatedTags) {
            FakeTag tag;

            TagType tagType;
            tagType.setName(helper.tagType);

            tag.setTagType(tagType);
            tag.setGid(helper.gid);
            tag.setRemoteId(helper.remoteId);
            tags << tag;
        }
    }

    Protocol::CreateItemCommandPtr createCommand(const PimItem &pimItem, const QDateTime &dt, const QSet<QByteArray> &parts, qint64 overrideSize = -1)
    {
        const qint64 size = overrideSize > -1 ? overrideSize : pimItem.size();

        auto cmd = Protocol::CreateItemCommandPtr::create();
        cmd->setCollection(Scope(pimItem.collectionId()));
        cmd->setItemSize(size);
        cmd->setRemoteId(pimItem.remoteId());
        cmd->setRemoteRevision(pimItem.remoteRevision());
        cmd->setMimeType(pimItem.mimeType().name());
        cmd->setGid(pimItem.gid());
        cmd->setDateTime(dt);
        cmd->setParts(parts);

        return cmd;
    }

    Protocol::FetchItemsResponsePtr createResponse(qint64 expectedId,
                                                   const PimItem &pimItem,
                                                   const QDateTime &datetime,
                                                   const QList<Protocol::StreamPayloadResponse> &parts,
                                                   qint64 overrideSize = -1)
    {
        const qint64 size = overrideSize > -1 ? overrideSize : pimItem.size();

        auto resp = Protocol::FetchItemsResponsePtr::create(expectedId);
        resp->setParentId(pimItem.collectionId());
        resp->setSize(size);
        resp->setRemoteId(pimItem.remoteId());
        resp->setRemoteRevision(pimItem.remoteRevision());
        resp->setMimeType(pimItem.mimeType().name());
        resp->setGid(pimItem.gid());
        resp->setMTime(datetime);
        resp->setParts(parts);
        resp->setAncestors({Protocol::Ancestor(4, QLatin1StringView("ColC"))});

        return resp;
    }

    TestScenario errorResponse(const QString &errorMsg)
    {
        auto response = Protocol::CreateItemResponsePtr::create();
        response->setError(1, errorMsg);
        return TestScenario::create(5, TestScenario::ServerCmd, response);
    }

private Q_SLOTS:
    void testItemCreate_data()
    {
        using Notifications = QList<Protocol::ItemChangeNotificationPtr>;

        QTest::addColumn<TestScenario::List>("scenarios");
        QTest::addColumn<Notifications>("notifications");
        QTest::addColumn<PimItem>("pimItem");
        QTest::addColumn<QList<FakePart>>("parts");
        QTest::addColumn<QList<Flag>>("flags");
        QTest::addColumn<QList<FakeTag>>("tags");
        QTest::addColumn<qint64>("uidnext");
        QTest::addColumn<QDateTime>("datetime");
        QTest::addColumn<bool>("expectFail");

        TestScenario::List scenarios;
        auto notification = Protocol::ItemChangeNotificationPtr::create();
        qint64 uidnext = 0;
        QDateTime datetime(QDate(2014, 05, 12), QTime(14, 46, 00), QTimeZone::UTC);
        PimItem pimItem;
        QList<FakePart> parts;
        QList<Flag> flags;
        QList<FakeTag> tags;

        pimItem.setCollectionId(4);
        pimItem.setSize(10);
        pimItem.setRemoteId(QStringLiteral("TEST-1"));
        pimItem.setRemoteRevision(QStringLiteral("1"));
        pimItem.setGid(QStringLiteral("TEST-1"));
        pimItem.setMimeType(MimeType::retrieveByName(QStringLiteral("application/octet-stream")));
        pimItem.setDatetime(datetime);
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), "0123456789", 10}});
        notification->setOperation(Protocol::ItemChangeNotification::Add);
        notification->setParentCollection(4);
        notification->setResource("akonadi_fake_resource_0");
        Protocol::FetchItemsResponse item;
        item.setId(-1);
        item.setRemoteId(QStringLiteral("TEST-1"));
        item.setRemoteRevision(QStringLiteral("1"));
        item.setMimeType(QStringLiteral("application/octet-stream"));
        notification->setItems({std::move(item)});
        notification->setSessionId(FakeAkonadiServer::instanceName().toLatin1());
        uidnext = 13;
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 10)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "0123456789"))
            << TestScenario::create(5,
                                    TestScenario::ServerCmd,
                                    createResponse(uidnext,
                                                   pimItem,
                                                   datetime,
                                                   {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 10), "0123456789")}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("single-part") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-2"), 20);
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), "Random Data", 11}, {QLatin1StringView("PLD:PLDTEST"), "Test Data", 9}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario()
            << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA", "PLD:PLDTEST"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5,
                                    TestScenario::ClientCmd,
                                    Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 11, 0)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "Random Data"))
            << TestScenario::create(5,
                                    TestScenario::ServerCmd,
                                    Protocol::StreamPayloadCommandPtr::create("PLD:PLDTEST", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5,
                                    TestScenario::ClientCmd,
                                    Protocol::StreamPayloadResponsePtr::create("PLD:PLDTEST", Protocol::PartMetaData("PLD:PLDTEST", 9, 0)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:PLDTEST", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:PLDTEST", "Test Data"))
            << TestScenario::create(5,
                                    TestScenario::ServerCmd,
                                    createResponse(uidnext,
                                                   pimItem,
                                                   datetime,
                                                   {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 11), "Random Data"),
                                                    Protocol::StreamPayloadResponse("PLD:PLDTEST", Protocol::PartMetaData("PLD:PLDTEST", 9), "Test Data")}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("multi-part") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        TestScenario inScenario;
        TestScenario outScenario;
        {
            auto cmd = Protocol::CreateItemCommandPtr::create();
            cmd->setCollection(Scope(100));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << errorResponse(QStringLiteral("Invalid parent collection"));
        QTest::newRow("invalid collection") << scenarios << Notifications{} << PimItem() << QList<FakePart>() << QList<Flag>() << QList<FakeTag>() << -1ll
                                            << QDateTime() << true;

        {
            auto cmd = Protocol::CreateItemCommandPtr::create();
            cmd->setCollection(Scope(6));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << errorResponse(QStringLiteral("Cannot append item into virtual collection"));
        QTest::newRow("virtual collection") << scenarios << Notifications{} << PimItem() << QList<FakePart>() << QList<Flag>() << QList<FakeTag>() << -1ll
                                            << QDateTime() << true;

        updatePimItem(pimItem, QStringLiteral("TEST-3"), 5);
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), "12345", 5}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}, 1))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 5)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "12345"))
            << TestScenario::create(
                   5,
                   TestScenario::ServerCmd,
                   createResponse(uidnext, pimItem, datetime, {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 5), "12345")}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("mismatch item sizes (smaller)")
            << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-4"), 10);
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}, 10))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 5)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "12345"))
            << TestScenario::create(5,
                                    TestScenario::ServerCmd,
                                    createResponse(uidnext,
                                                   pimItem,
                                                   datetime,
                                                   {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 5), "12345")},
                                                   10))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("mismatch item sizes (bigger)") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime
                                                      << false;

        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 5)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "123"))
            << errorResponse(QStringLiteral("Payload size mismatch"));
        QTest::newRow("incomplete part data") << scenarios << Notifications{} << PimItem() << QList<FakePart>() << QList<Flag>() << QList<FakeTag>() << -1ll
                                              << QDateTime() << true;

        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 4)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", "1234567890"))
            << errorResponse(QStringLiteral("Payload size mismatch"));
        QTest::newRow("part data larger than advertised")
            << scenarios << Notifications{} << PimItem() << QList<FakePart>() << QList<Flag>() << QList<FakeTag>() << -1ll << QDateTime() << true;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-5"), 0);
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), QByteArray(), 0}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 0)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", QByteArray()))
            << TestScenario::create(5,
                                    TestScenario::ServerCmd,
                                    createResponse(uidnext,
                                                   pimItem,
                                                   datetime,
                                                   {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 0), QByteArray())}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("empty payload part") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-8"), 1);
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), QByteArray("\0", 1), 1}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 1)))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", QByteArray("\0", 1)))
            << TestScenario::create(5,
                                    TestScenario::ServerCmd,
                                    createResponse(uidnext,
                                                   pimItem,
                                                   datetime,
                                                   {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", 1), QByteArray("\0", 1))}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("part data will null character")
            << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        const QString utf8String = QStringLiteral("äöüß@€µøđ¢©®");
        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-9"), utf8String.toUtf8().size());
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), utf8String.toUtf8(), utf8String.toUtf8().size()}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5,
                                    TestScenario::ClientCmd,
                                    Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", parts.first().datasize())))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", utf8String.toUtf8()))
            << TestScenario::create(
                   5,
                   TestScenario::ServerCmd,
                   createResponse(
                       uidnext,
                       pimItem,
                       datetime,
                       {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", utf8String.toUtf8().size()), utf8String.toUtf8())}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("utf8 part data") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        const QByteArray hugeData = QByteArray("a").repeated(1 << 20);
        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-10"), 1 << 20);
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), hugeData, 1 << 20}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5,
                                    TestScenario::ClientCmd,
                                    Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", parts.first().datasize())))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", hugeData))
            << TestScenario::create(
                   5,
                   TestScenario::ServerCmd,
                   createResponse(uidnext,
                                  pimItem,
                                  datetime,
                                  {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", parts.first().datasize()), hugeData)}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("huge part data") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        const QByteArray dataWithNewLines = "Bernard, Bernard, Bernard, Bernard, look, look Bernard!\nWHAT!!!!!!!\nI'm a prostitute robot from the future!";
        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-11"), dataWithNewLines.size());
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), dataWithNewLines, dataWithNewLines.size()}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5,
                                    TestScenario::ClientCmd,
                                    Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", parts.first().datasize())))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", dataWithNewLines))
            << TestScenario::create(
                   5,
                   TestScenario::ServerCmd,
                   createResponse(uidnext,
                                  pimItem,
                                  datetime,
                                  {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", dataWithNewLines.size()), dataWithNewLines)}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("data with newlines") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        const QByteArray lotsOfNewlines = QByteArray("\n").repeated(1 << 20);
        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-12"), lotsOfNewlines.size());
        updateParts(parts, {{QLatin1StringView("PLD:DATA"), lotsOfNewlines, lotsOfNewlines.size()}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios
            << FakeAkonadiServer::loginScenario() << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:DATA"}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::MetaData))
            << TestScenario::create(5,
                                    TestScenario::ClientCmd,
                                    Protocol::StreamPayloadResponsePtr::create("PLD:DATA", Protocol::PartMetaData("PLD:DATA", parts.first().datasize())))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::StreamPayloadCommandPtr::create("PLD:DATA", Protocol::StreamPayloadCommand::Data))
            << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:DATA", lotsOfNewlines))
            << TestScenario::create(
                   5,
                   TestScenario::ServerCmd,
                   createResponse(uidnext,
                                  pimItem,
                                  datetime,
                                  {Protocol::StreamPayloadResponse("PLD:DATA", Protocol::PartMetaData("PLD:DATA", parts.first().datasize()), lotsOfNewlines)}))
            << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("data with lots of newlines") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime
                                                    << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-13"), 20);
        updateParts(parts, {{QLatin1StringView("PLD:NEWPARTTYPE1"), "0123456789", 10}, {QLatin1StringView("PLD:NEWPARTTYPE2"), "9876543210", 10}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario()
                  << TestScenario::create(5, TestScenario::ClientCmd, createCommand(pimItem, datetime, {"PLD:NEWPARTTYPE1", "PLD:NEWPARTTYPE2"}))
                  << TestScenario::create(5,
                                          TestScenario::ServerCmd,
                                          Protocol::StreamPayloadCommandPtr::create("PLD:NEWPARTTYPE1", Protocol::StreamPayloadCommand::MetaData))
                  << TestScenario::create(5,
                                          TestScenario::ClientCmd,
                                          Protocol::StreamPayloadResponsePtr::create("PLD:NEWPARTTYPE1", Protocol::PartMetaData("PLD:NEWPARTTYPE1", 10)))
                  << TestScenario::create(5,
                                          TestScenario::ServerCmd,
                                          Protocol::StreamPayloadCommandPtr::create("PLD:NEWPARTTYPE1", Protocol::StreamPayloadCommand::Data))
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:NEWPARTTYPE2", "0123456789"))
                  << TestScenario::create(5,
                                          TestScenario::ServerCmd,
                                          Protocol::StreamPayloadCommandPtr::create("PLD:NEWPARTTYPE2", Protocol::StreamPayloadCommand::MetaData))
                  << TestScenario::create(5,
                                          TestScenario::ClientCmd,
                                          Protocol::StreamPayloadResponsePtr::create("PLD:NEWPARTTYPE2", Protocol::PartMetaData("PLD:NEWPARTTYPE2", 10)))
                  << TestScenario::create(5,
                                          TestScenario::ServerCmd,
                                          Protocol::StreamPayloadCommandPtr::create("PLD:NEWPARTTYPE2", Protocol::StreamPayloadCommand::Data))
                  << TestScenario::create(5, TestScenario::ClientCmd, Protocol::StreamPayloadResponsePtr::create("PLD:NEWPARTTYPE2", "9876543210"))
                  << TestScenario::create(
                         5,
                         TestScenario::ServerCmd,
                         createResponse(uidnext,
                                        pimItem,
                                        datetime,
                                        {Protocol::StreamPayloadResponse("PLD:NEWPARTTYPE1", Protocol::PartMetaData("PLD:NEWPARTTYPE1", 10), "0123456789"),
                                         Protocol::StreamPayloadResponse("PLD:NEWPARTTYPE2", Protocol::PartMetaData("PLD:NEWPARTTYPE2", 10), "9876543210")}))
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("non-existent part types") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime
                                                 << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-14"), 0);
        updateParts(parts, {});
        updateFlags(flags, QStringList() << QStringLiteral("\\SEEN") << QStringLiteral("\\RANDOM"));
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setFlags({"\\SEEN", "\\RANDOM"});
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setFlags({"\\SEEN", "\\RANDOM"});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << outScenario
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with flags") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-15"), 0);
        updateFlags(flags, {});
        updateTags(tags, {{QLatin1StringView("PLAIN"), QLatin1StringView("TAG-1")}, {QLatin1StringView("PLAIN"), QLatin1StringView("TAG-2")}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setTags(Scope(Scope::Gid, {QLatin1StringView("TAG-1"), QLatin1StringView("TAG-2")}));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setTags({Protocol::FetchTagsResponse(2, "TAG-1", "PLAIN"), Protocol::FetchTagsResponse(3, "TAG-2", "PLAIN")});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << outScenario
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with non-existent tags (GID)")
            << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-16"), 0);
        updateTags(tags, {{QLatin1StringView("PLAIN"), QLatin1StringView("TAG-3")}, {QLatin1StringView("PLAIN"), QLatin1StringView("TAG-4")}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setTags(Scope(Scope::Rid, {QLatin1StringView("TAG-3"), QLatin1StringView("TAG-4")}));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setTags({Protocol::FetchTagsResponse(4, "TAG-3", "PLAIN"), Protocol::FetchTagsResponse(5, "TAG-4", "PLAIN")});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << FakeAkonadiServer::selectResourceScenario(QStringLiteral("akonadi_fake_resource_0")) << inScenario
                  << outScenario << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with non-existent tags (RID)")
            << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-17"), 0);
        updateNotifcationEntity(notification, pimItem);
        updateTags(tags, {{QLatin1StringView("PLAIN"), QLatin1StringView("TAG-1")}, {QLatin1StringView("PLAIN"), QLatin1StringView("TAG-2")}});
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setTags(Scope(Scope::Rid, {QLatin1StringView("TAG-1"), QLatin1StringView("TAG-2")}));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setTags({Protocol::FetchTagsResponse(2, "TAG-1", "PLAIN"), Protocol::FetchTagsResponse(3, "TAG-2", "PLAIN")});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << FakeAkonadiServer::selectResourceScenario(QStringLiteral("akonadi_fake_resource_0")) << inScenario
                  << outScenario << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with existing tags (RID)")
            << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-18"), 0);
        updateNotifcationEntity(notification, pimItem);
        updateTags(tags, {{QLatin1StringView("PLAIN"), QLatin1StringView("TAG-3")}, {QLatin1StringView("PLAIN"), QLatin1StringView("TAG-4")}});
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setTags(Scope(Scope::Gid, {QLatin1StringView("TAG-3"), QLatin1StringView("TAG-4")}));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setTags({Protocol::FetchTagsResponse(4, "TAG-3", "PLAIN"), Protocol::FetchTagsResponse(5, "TAG-4", "PLAIN")});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << outScenario
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with existing tags (GID)")
            << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-19"), 0);
        updateFlags(flags, QStringList() << QStringLiteral("\\SEEN") << QStringLiteral("$FLAG"));
        updateTags(tags, {{QLatin1StringView("PLAIN"), QLatin1StringView("TAG-1")}, {QLatin1StringView("PLAIN"), QLatin1StringView("TAG-2")}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setTags(Scope(Scope::Gid, {QLatin1StringView("TAG-1"), QLatin1StringView("TAG-2")}));
            cmd->setFlags({"\\SEEN", "$FLAG"});
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setTags({Protocol::FetchTagsResponse(2, "TAG-1", "PLAIN"), Protocol::FetchTagsResponse(3, "TAG-2", "PLAIN")});
            rsp->setFlags({"\\SEEN", "$FLAG"});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << outScenario
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with flags and tags") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime
                                                  << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-20"), 0);
        updateFlags(flags, {});
        updateTags(tags, {{QLatin1StringView("PLAIN"), utf8String}});
        updateNotifcationEntity(notification, pimItem);
        ++uidnext;
        {
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setTags(Scope(Scope::Gid, {utf8String}));
            inScenario = TestScenario::create(5, TestScenario::ClientCmd, cmd);

            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            rsp->setTags({Protocol::FetchTagsResponse(6, utf8String.toUtf8(), "PLAIN")});
            outScenario = TestScenario::create(5, TestScenario::ServerCmd, rsp);
        }
        scenarios.clear();
        scenarios << FakeAkonadiServer::loginScenario() << inScenario << outScenario
                  << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        QTest::newRow("item with UTF-8 tag") << scenarios << Notifications{notification} << pimItem << parts << flags << tags << uidnext << datetime << false;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-21"), 0);
        updateFlags(flags, {});
        updateTags(tags, {});
        pimItem.setGid(QStringLiteral("GID-21"));
        updateNotifcationEntity(notification, pimItem);
        scenarios = FakeAkonadiServer::loginScenario();
        // Create a normal item with RID
        {
            ++uidnext;
            auto cmd = createCommand(pimItem, datetime, {});
            scenarios << TestScenario::create(5, TestScenario::ClientCmd, cmd);
            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            scenarios << TestScenario::create(5, TestScenario::ServerCmd, rsp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        }
        // Create the same item again (no merging, so it will just be created)
        {
            ++uidnext;
            auto cmd = createCommand(pimItem, datetime, {});
            scenarios << TestScenario::create(6, TestScenario::ClientCmd, cmd);
            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            scenarios << TestScenario::create(6, TestScenario::ServerCmd, rsp)
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        }
        // Now try to create the item once again, but in merge mode, we should fail now
        {
            ++uidnext;
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setMergeModes(Protocol::CreateItemCommand::RemoteID);
            scenarios << TestScenario::create(7, TestScenario::ClientCmd, cmd);
            auto rsp = Protocol::CreateItemResponsePtr::create();
            rsp->setError(1, QStringLiteral("Multiple merge candidates"));
            scenarios << TestScenario::create(7, TestScenario::ServerCmd, rsp);
        }
        Notifications notifications = {notification, Protocol::ItemChangeNotificationPtr::create(*notification)};
        QTest::newRow("multiple merge candidates (RID)") << scenarios << notifications << pimItem << parts << flags << tags << uidnext << datetime << true;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-22"), 0);
        pimItem.setGid(QStringLiteral("GID-22"));
        updateNotifcationEntity(notification, pimItem);
        scenarios = FakeAkonadiServer::loginScenario();
        // Create a normal item with GID
        {
            // Don't increase uidnext, we will reuse the one from previous test,
            // since that did not actually create a new Item
            auto cmd = createCommand(pimItem, datetime, {});
            scenarios << TestScenario::create(5, TestScenario::ClientCmd, cmd);
            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            scenarios << TestScenario::create(5, TestScenario::ServerCmd, rsp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        }
        // Create the same item again (no merging, so it will just be created)
        {
            ++uidnext;
            auto cmd = createCommand(pimItem, datetime, {});
            scenarios << TestScenario::create(6, TestScenario::ClientCmd, cmd);
            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            scenarios << TestScenario::create(6, TestScenario::ServerCmd, rsp)
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        }
        // Now try to create the item once again, but in merge mode, we should fail now
        {
            ++uidnext;
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setMergeModes(Protocol::CreateItemCommand::GID);
            scenarios << TestScenario::create(7, TestScenario::ClientCmd, cmd);
            auto rsp = Protocol::CreateItemResponsePtr::create();
            rsp->setError(1, QStringLiteral("Multiple merge candidates"));
            scenarios << TestScenario::create(7, TestScenario::ServerCmd, rsp);
        }
        notifications = {notification, Protocol::ItemChangeNotificationPtr::create(*notification)};
        QTest::newRow("multiple merge candidates (GID)") << scenarios << notifications << pimItem << parts << flags << tags << uidnext << datetime << true;

        notification = Protocol::ItemChangeNotificationPtr::create(*notification);
        updatePimItem(pimItem, QStringLiteral("TEST-23"), 0);
        pimItem.setGid(QString());
        updateNotifcationEntity(notification, pimItem);
        scenarios = FakeAkonadiServer::loginScenario();
        // Create a normal item with RID, but with empty GID
        {
            // Don't increase uidnext, we will reuse the one from previous test,
            // since that did not actually create a new Item
            auto cmd = createCommand(pimItem, datetime, {});
            scenarios << TestScenario::create(5, TestScenario::ClientCmd, cmd);
            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            scenarios << TestScenario::create(5, TestScenario::ServerCmd, rsp)
                      << TestScenario::create(5, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        }
        // Merge by GID - should not create a new Item but actually merge by RID,
        // since an item with matching RID but empty GID exists
        {
            ++uidnext;
            pimItem.setGid(QStringLiteral("GID-23"));
            auto cmd = createCommand(pimItem, datetime, {});
            cmd->setMergeModes(Protocol::CreateItemCommand::GID);
            scenarios << TestScenario::create(6, TestScenario::ClientCmd, cmd);
            auto rsp = createResponse(uidnext, pimItem, datetime, {});
            scenarios << TestScenario::create(6, TestScenario::ServerCmd, rsp)
                      << TestScenario::create(6, TestScenario::ServerCmd, Protocol::CreateItemResponsePtr::create());
        }
        notifications = {notification, Protocol::ItemChangeNotificationPtr::create(*notification)};
        QTest::newRow("merge into empty GID if RID matches") << scenarios << notifications << pimItem << parts << flags << tags << uidnext << datetime << false;
    }

    void testItemCreate()
    {
        QFETCH(TestScenario::List, scenarios);
        QFETCH(QList<Protocol::ItemChangeNotificationPtr>, notifications);
        QFETCH(PimItem, pimItem);
        QFETCH(QList<FakePart>, parts);
        QFETCH(QList<Flag>, flags);
        QFETCH(QList<FakeTag>, tags);
        QFETCH(qint64, uidnext);
        QFETCH(bool, expectFail);

        mAkonadi.setScenarios(scenarios);
        mAkonadi.runTest();

        auto notificationSpy = mAkonadi.notificationSpy();

        QTRY_COMPARE(notificationSpy->count(), notifications.count());
        for (int i = 0; i < notifications.count(); ++i) {
            const auto incomingNtfs = notificationSpy->at(i).first().value<Protocol::ChangeNotificationList>();
            QCOMPARE(incomingNtfs.count(), 1);
            const auto itemNotification = incomingNtfs.at(0).staticCast<Protocol::ItemChangeNotification>();

            QVERIFY(AkTest::compareNotifications(itemNotification, notifications.at(i), QFlag(AkTest::NtfAll & ~AkTest::NtfEntities)));
            QCOMPARE(itemNotification->items().count(), notifications.at(i)->items().count());
        }

        const PimItem actualItem = PimItem::retrieveById(uidnext);
        if (expectFail) {
            QVERIFY(!actualItem.isValid());
        } else {
            QVERIFY(actualItem.isValid());
            QCOMPARE(actualItem.remoteId(), pimItem.remoteId());
            QCOMPARE(actualItem.remoteRevision(), pimItem.remoteRevision());
            QCOMPARE(actualItem.gid(), pimItem.gid());
            QCOMPARE(actualItem.size(), pimItem.size());
            QCOMPARE(actualItem.datetime(), pimItem.datetime());
            QCOMPARE(actualItem.collectionId(), pimItem.collectionId());
            QCOMPARE(actualItem.mimeTypeId(), pimItem.mimeTypeId());

            const auto actualFlags = actualItem.flags() | AkRanges::Actions::toQList;
            QCOMPARE(actualFlags.count(), flags.count());
            for (const Flag &flag : std::as_const(flags)) {
                const auto actualFlagIter = std::find_if(actualFlags.constBegin(), actualFlags.constEnd(), [flag](Flag const &actualFlag) {
                    return flag.name() == actualFlag.name();
                });
                QVERIFY(actualFlagIter != actualFlags.constEnd());
                const Flag actualFlag = *actualFlagIter;
                QVERIFY(actualFlag.isValid());
            }

            const auto actualTags = actualItem.tags() | AkRanges::Actions::toQList;
            QCOMPARE(actualTags.count(), tags.count());
            for (const FakeTag &tag : std::as_const(tags)) {
                const auto actualTagIter = std::find_if(actualTags.constBegin(), actualTags.constEnd(), [tag](Tag const &actualTag) {
                    return tag.gid() == actualTag.gid();
                });

                QVERIFY(actualTagIter != actualTags.constEnd());
                const Tag actualTag = *actualTagIter;
                QVERIFY(actualTag.isValid());
                QCOMPARE(actualTag.tagType().name(), tag.tagType().name());
                QCOMPARE(actualTag.gid(), tag.gid());
                if (!tag.remoteId().isEmpty()) {
                    SelectQueryBuilder<TagRemoteIdResourceRelation> qb;
                    qb.addValueCondition(TagRemoteIdResourceRelation::resourceIdFullColumnName(), Query::Equals, QLatin1StringView("akonadi_fake_resource_0"));
                    qb.addValueCondition(TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, actualTag.id());
                    QVERIFY(qb.exec());
                    QCOMPARE(qb.result().size(), 1);
                    QCOMPARE(qb.result().at(0).remoteId(), tag.remoteId());
                }
            }

            const auto actualParts = actualItem.parts() | AkRanges::Actions::toQList;
            QCOMPARE(actualParts.count(), parts.count());
            for (const FakePart &part : std::as_const(parts)) {
                const auto actualPartIter = std::find_if(actualParts.constBegin(), actualParts.constEnd(), [part](Part const &actualPart) {
                    return part.partType().ns() == actualPart.partType().ns() && part.partType().name() == actualPart.partType().name();
                });

                QVERIFY(actualPartIter != actualParts.constEnd());
                const Part actualPart = *actualPartIter;
                QVERIFY(actualPart.isValid());
                QCOMPARE(QString::fromUtf8(actualPart.data()), QString::fromUtf8(part.data()));
                QCOMPARE(actualPart.data(), part.data());
                QCOMPARE(actualPart.datasize(), part.datasize());
                QCOMPARE(actualPart.storage(), part.storage());
            }
        }
    }
};

AKTEST_FAKESERVER_MAIN(ItemCreateHandlerTest)

#include "itemcreatehandlertest.moc"
