/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "protocoltest.h"

#include "private/scope_p.h"

#include <QTest>

using namespace Akonadi;
using namespace Akonadi::Protocol;

void ProtocolTest::testFactory_data()
{
    QTest::addColumn<Command::Type>("type");
    QTest::addColumn<bool>("response");
    QTest::addColumn<bool>("success");

    QTest::newRow("invalid cmd") << Command::Invalid << false << false;
    QTest::newRow("invalid resp") << Command::Invalid << true << false;
    QTest::newRow("hello cmd") << Command::Hello << false << false;
    QTest::newRow("hello resp") << Command::Hello << true << true;
    QTest::newRow("login cmd") << Command::Login << false << true;
    QTest::newRow("login resp") << Command::Login << true << true;
    QTest::newRow("logout cmd") << Command::Logout << false << true;
    QTest::newRow("logout resp") << Command::Logout << true << true;
    QTest::newRow("transaction cmd") << Command::Transaction << false << true;
    QTest::newRow("transaction resp") << Command::Transaction << true << true;
    QTest::newRow("createItem cmd") << Command::CreateItem << false << true;
    QTest::newRow("createItem resp") << Command::CreateItem << true << true;
    QTest::newRow("copyItems cmd") << Command::CopyItems << false << true;
    QTest::newRow("copyItems resp") << Command::CopyItems << true << true;
    QTest::newRow("deleteItems cmd") << Command::DeleteItems << false << true;
    QTest::newRow("deleteItems resp") << Command::DeleteItems << true << true;
    QTest::newRow("fetchItems cmd") << Command::FetchItems << false << true;
    QTest::newRow("fetchItems resp") << Command::FetchItems << true << true;
    QTest::newRow("linkItems cmd") << Command::LinkItems << false << true;
    QTest::newRow("linkItems resp") << Command::LinkItems << true << true;
    QTest::newRow("modifyItems cmd") << Command::ModifyItems << false << true;
    QTest::newRow("modifyItems resp") << Command::ModifyItems << true << true;
    QTest::newRow("moveItems cmd") << Command::MoveItems << false << true;
    QTest::newRow("moveItems resp") << Command::MoveItems << true << true;
    QTest::newRow("createCollection cmd") << Command::CreateCollection << false << true;
    QTest::newRow("createCollection resp") << Command::CreateCollection << true << true;
    QTest::newRow("copyCollection cmd") << Command::CopyCollection << false << true;
    QTest::newRow("copyCollection resp") << Command::CopyCollection << true << true;
    QTest::newRow("deleteCollection cmd") << Command::DeleteCollection << false << true;
    QTest::newRow("deleteCollection resp") << Command::DeleteCollection << true << true;
    QTest::newRow("fetchCollections cmd") << Command::FetchCollections << false << true;
    QTest::newRow("fetchCollections resp") << Command::FetchCollections << true << true;
    QTest::newRow("fetchCollectionStats cmd") << Command::FetchCollectionStats << false << true;
    QTest::newRow("fetchCollectionStats resp") << Command::FetchCollectionStats << false << true;
    QTest::newRow("modifyCollection cmd") << Command::ModifyCollection << false << true;
    QTest::newRow("modifyCollection resp") << Command::ModifyCollection << true << true;
    QTest::newRow("moveCollection cmd") << Command::MoveCollection << false << true;
    QTest::newRow("moveCollection resp") << Command::MoveCollection << true << true;
    QTest::newRow("search cmd") << Command::Search << false << true;
    QTest::newRow("search resp") << Command::Search << true << true;
    QTest::newRow("searchResult cmd") << Command::SearchResult << false << true;
    QTest::newRow("searchResult resp") << Command::SearchResult << true << true;
    QTest::newRow("storeSearch cmd") << Command::StoreSearch << false << true;
    QTest::newRow("storeSearch resp") << Command::StoreSearch << true << true;
    QTest::newRow("createTag cmd") << Command::CreateTag << false << true;
    QTest::newRow("createTag resp") << Command::CreateTag << true << true;
    QTest::newRow("deleteTag cmd") << Command::DeleteTag << false << true;
    QTest::newRow("deleteTag resp") << Command::DeleteTag << true << true;
    QTest::newRow("fetchTags cmd") << Command::FetchTags << false << true;
    QTest::newRow("fetchTags resp") << Command::FetchTags << true << true;
    QTest::newRow("modifyTag cmd") << Command::ModifyTag << false << true;
    QTest::newRow("modifyTag resp") << Command::ModifyTag << true << true;
    QTest::newRow("fetchRelations cmd") << Command::FetchRelations << false << true;
    QTest::newRow("fetchRelations resp") << Command::FetchRelations << true << true;
    QTest::newRow("modifyRelation cmd") << Command::ModifyRelation << false << true;
    QTest::newRow("modifyRelation resp") << Command::ModifyRelation << true << true;
    QTest::newRow("removeRelations cmd") << Command::RemoveRelations << false << true;
    QTest::newRow("removeRelations resp") << Command::RemoveRelations << true << true;
    QTest::newRow("selectResource cmd") << Command::SelectResource << false << true;
    QTest::newRow("selectResource resp") << Command::SelectResource << true << true;
    QTest::newRow("streamPayload cmd") << Command::StreamPayload << false << true;
    QTest::newRow("streamPayload resp") << Command::StreamPayload << true << true;
    QTest::newRow("itemChangeNotification cmd") << Command::ItemChangeNotification << false << true;
    QTest::newRow("itemChangeNotification resp") << Command::ItemChangeNotification << true << false;
    QTest::newRow("collectionChangeNotification cmd") << Command::CollectionChangeNotification << false << true;
    QTest::newRow("collectionChangeNotification resp") << Command::CollectionChangeNotification << true << false;
    QTest::newRow("tagChangeNotification cmd") << Command::TagChangeNotification << false << true;
    QTest::newRow("tagChangENotification resp") << Command::TagChangeNotification << true << false;
    QTest::newRow("relationChangeNotification cmd") << Command::RelationChangeNotification << false << true;
    QTest::newRow("relationChangeNotification resp") << Command::RelationChangeNotification << true << false;
    QTest::newRow("_responseBit cmd") << Command::_ResponseBit << false << false;
    QTest::newRow("_responseBit resp") << Command::_ResponseBit << true << false;
}

void ProtocolTest::testFactory()
{
    QFETCH(Command::Type, type);
    QFETCH(bool, response);
    QFETCH(bool, success);

    CommandPtr result;
    if (response) {
        result = Factory::response(type);
    } else {
        result = Factory::command(type);
    }

    QCOMPARE(result->isValid(), success);
    QCOMPARE(result->isResponse(), response);
    if (success) {
        QCOMPARE(result->type(), type);
    }
}



void ProtocolTest::testCommand()
{
    // There is no way to construct a valid Command directly
    auto cmd = CommandPtr::create();
    QCOMPARE(cmd->type(), Command::Invalid);
    QVERIFY(!cmd->isValid());
    QVERIFY(!cmd->isResponse());

    CommandPtr cmdTest = serializeAndDeserialize(cmd);
    QCOMPARE(cmdTest->type(), Command::Invalid);
    QVERIFY(!cmd->isValid());
    QVERIFY(!cmd->isResponse());
}

void ProtocolTest::testResponse_data()
{
    QTest::addColumn<bool>("isError");
    QTest::addColumn<int>("errorCode");
    QTest::addColumn<QString>("errorString");

    QTest::newRow("no error") << false << 0 << QString();
    QTest::newRow("error") << true << 10 << QStringLiteral("Oh noes, there was an error!");
}

void ProtocolTest::testResponse()
{
    QFETCH(bool, isError);
    QFETCH(int, errorCode);
    QFETCH(QString, errorString);

    Response response;
    if (isError) {
        response.setError(errorCode, errorString);
    }

    const auto res = serializeAndDeserialize(ResponsePtr::create(response));
    QCOMPARE(res->type(), Command::Invalid);
    QVERIFY(!res->isValid());
    QVERIFY(res->isResponse());
    QCOMPARE(res->isError(), isError);
    QCOMPARE(res->errorCode(), errorCode);
    QCOMPARE(res->errorMessage(), errorString);
    QVERIFY(*res == response);
    const bool notEquals = (*res != response);
    QVERIFY(!notEquals);
}

void ProtocolTest::testAncestor()
{
    Ancestor in;
    in.setId(42);
    in.setRemoteId(QStringLiteral("remoteId"));
    in.setName(QStringLiteral("Col 42"));
    in.setAttributes({{ "Attr1", "Val 1" }, { "Attr2", "Röndom útéef řetězec" }});

    const Ancestor out = serializeAndDeserialize(in);
    QCOMPARE(out.id(), 42);
    QCOMPARE(out.remoteId(), QStringLiteral("remoteId"));
    QCOMPARE(out.name(), QStringLiteral("Col 42"));
    QCOMPARE(out.attributes(), Attributes({{ "Attr1", "Val 1" }, { "Attr2", "Röndom útéef řetězec" }}));
    QVERIFY(out == in);
    const bool notEquals = (out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testFetchScope_data()
{
    QTest::addColumn<bool>("fullPayload");
    QTest::addColumn<QVector<QByteArray>>("requestedParts");
    QTest::addColumn<QVector<QByteArray>>("expectedParts");
    QTest::addColumn<QVector<QByteArray>>("expectedPayloads");
    QTest::newRow("full payload (via flag") << true
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR" }
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR", "PLD:RFC822" }
                                << QVector<QByteArray>{ "PLD:HEAD", "PLD:RFC822" };
    QTest::newRow("full payload (via part name") << false
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR", "PLD:RFC822" }
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR", "PLD:RFC822" }
                                << QVector<QByteArray>{ "PLD:HEAD", "PLD:RFC822" };
    QTest::newRow("full payload (via both") << true
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR", "PLD:RFC822" }
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR", "PLD:RFC822" }
                                << QVector<QByteArray>{ "PLD:HEAD", "PLD:RFC822" };
    QTest::newRow("without full payload") << false
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR" }
                                << QVector<QByteArray>{ "PLD:HEAD", "ATR:MYATR" }
                                << QVector<QByteArray>{ "PLD:HEAD" };
}


void ProtocolTest::testFetchScope()
{
    QFETCH(bool, fullPayload);
    QFETCH(QVector<QByteArray>, requestedParts);
    QFETCH(QVector<QByteArray>, expectedParts);
    QFETCH(QVector<QByteArray>, expectedPayloads);

    ItemFetchScope in;
    for (unsigned i = ItemFetchScope::CacheOnly; i <= ItemFetchScope::VirtReferences; i = i << 1) {
        QVERIFY(!in.fetch(static_cast<ItemFetchScope::FetchFlag>(i)));
    }
    QVERIFY(in.fetch(ItemFetchScope::None));

    in.setRequestedParts(requestedParts);
    in.setChangedSince(QDateTime(QDate(2015, 8, 10), QTime(23, 52, 20), Qt::UTC));
    in.setAncestorDepth(ItemFetchScope::AllAncestors);
    in.setFetch(ItemFetchScope::CacheOnly);
    in.setFetch(ItemFetchScope::CheckCachedPayloadPartsOnly);
    in.setFetch(ItemFetchScope::FullPayload, fullPayload);
    in.setFetch(ItemFetchScope::AllAttributes);
    in.setFetch(ItemFetchScope::Size);
    in.setFetch(ItemFetchScope::MTime);
    in.setFetch(ItemFetchScope::RemoteRevision);
    in.setFetch(ItemFetchScope::IgnoreErrors);
    in.setFetch(ItemFetchScope::Flags);
    in.setFetch(ItemFetchScope::RemoteID);
    in.setFetch(ItemFetchScope::GID);
    in.setFetch(ItemFetchScope::Tags);
    in.setFetch(ItemFetchScope::Relations);
    in.setFetch(ItemFetchScope::VirtReferences);

    const ItemFetchScope out = serializeAndDeserialize(in);
    QCOMPARE(out.requestedParts(), expectedParts);
    QCOMPARE(out.requestedPayloads(), expectedPayloads);
    QCOMPARE(out.changedSince(), QDateTime(QDate(2015, 8, 10), QTime(23, 52, 20), Qt::UTC));
    QCOMPARE(out.ancestorDepth(), ItemFetchScope::AllAncestors);
    QCOMPARE(out.fetch(ItemFetchScope::None), false);
    QCOMPARE(out.cacheOnly(), true);
    QCOMPARE(out.checkCachedPayloadPartsOnly(), true);
    QCOMPARE(out.fullPayload(), fullPayload);
    QCOMPARE(out.allAttributes(), true);
    QCOMPARE(out.fetchSize(), true);
    QCOMPARE(out.fetchMTime(), true);
    QCOMPARE(out.fetchRemoteRevision(), true);
    QCOMPARE(out.ignoreErrors(), true);
    QCOMPARE(out.fetchFlags(), true);
    QCOMPARE(out.fetchRemoteId(), true);
    QCOMPARE(out.fetchGID(), true);
    QCOMPARE(out.fetchRelations(), true);
    QCOMPARE(out.fetchVirtualReferences(), true);
}

void ProtocolTest::testScopeContext_data()
{
    QTest::addColumn<qint64>("colId");
    QTest::addColumn<QString>("colRid");
    QTest::addColumn<qint64>("tagId");
    QTest::addColumn<QString>("tagRid");

    QTest::newRow("collection - id") << 42LL << QString()
                                     << 0LL << QString();
    QTest::newRow("collection - rid") << 0LL << QStringLiteral("rid")
                                      << 0LL << QString();
    QTest::newRow("collection - both") << 42LL << QStringLiteral("rid")
                                       << 0LL << QString();

    QTest::newRow("tag - id") << 0LL << QString()
                              << 42LL << QString();
    QTest::newRow("tag - rid") << 0LL << QString()
                               << 0LL << QStringLiteral("rid");
    QTest::newRow("tag - both") << 0LL << QString()
                                << 42LL << QStringLiteral("rid");

    QTest::newRow("both - id") << 42LL << QString()
                               << 10LL << QString();
    QTest::newRow("both - rid") << 0LL << QStringLiteral("colRid")
                                << 0LL << QStringLiteral("tagRid");
    QTest::newRow("col - id, tag - rid") << 42LL << QString()
                                         << 0LL << QStringLiteral("tagRid");
    QTest::newRow("col - rid, tag - id") << 0LL << QStringLiteral("colRid")
                                         << 42LL << QString();
    QTest::newRow("both - both") << 42LL << QStringLiteral("colRid")
                                 << 10LL << QStringLiteral("tagRid");
}

void ProtocolTest::testScopeContext()
{
    QFETCH(qint64, colId);
    QFETCH(QString, colRid);
    QFETCH(qint64, tagId);
    QFETCH(QString, tagRid);

    const bool hasColId = colId > 0;
    const bool hasColRid = !colRid.isEmpty();
    const bool hasTagId = tagId > 0;
    const bool hasTagRid = !tagRid.isEmpty();

    ScopeContext in;
    QVERIFY(in.isEmpty());
    if (hasColId) {
        in.setContext(ScopeContext::Collection, colId);
    }
    if (hasColRid) {
        in.setContext(ScopeContext::Collection, colRid);
    }
    if (hasTagId) {
        in.setContext(ScopeContext::Tag, tagId);
    }
    if (hasTagRid) {
        in.setContext(ScopeContext::Tag, tagRid);
    }

    QCOMPARE(in.hasContextId(ScopeContext::Any), false);
    QCOMPARE(in.hasContextRID(ScopeContext::Any), false);
    QEXPECT_FAIL("collection - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(in.hasContextId(ScopeContext::Collection), hasColId);
    QCOMPARE(in.hasContextRID(ScopeContext::Collection), hasColRid);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("tag - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(in.hasContextId(ScopeContext::Tag), hasTagId);
    QCOMPARE(in.hasContextRID(ScopeContext::Tag), hasTagRid);
    QVERIFY(!in.isEmpty());

    ScopeContext out = serializeAndDeserialize(in);
    QCOMPARE(out.isEmpty(), false);
    QEXPECT_FAIL("collection - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.hasContextId(ScopeContext::Collection), hasColId);
    QEXPECT_FAIL("collection - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.contextId(ScopeContext::Collection), colId);
    QCOMPARE(out.hasContextRID(ScopeContext::Collection), hasColRid);
    QCOMPARE(out.contextRID(ScopeContext::Collection), colRid);
    QEXPECT_FAIL("tag - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.hasContextId(ScopeContext::Tag), hasTagId);
    QEXPECT_FAIL("tag - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.contextId(ScopeContext::Tag), tagId);
    QCOMPARE(out.hasContextRID(ScopeContext::Tag), hasTagRid);
    QCOMPARE(out.contextRID(ScopeContext::Tag), tagRid);
    QCOMPARE(out, in);
    const bool notEquals = (out != in);
    QVERIFY(!notEquals);

    // Clearing "any" should not do anything
    out.clearContext(ScopeContext::Any);
    QEXPECT_FAIL("collection - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.hasContextId(ScopeContext::Collection), hasColId);
    QEXPECT_FAIL("collection - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.contextId(ScopeContext::Collection), colId);
    QCOMPARE(out.hasContextRID(ScopeContext::Collection), hasColRid);
    QCOMPARE(out.contextRID(ScopeContext::Collection), colRid);
    QEXPECT_FAIL("tag - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.hasContextId(ScopeContext::Tag), hasTagId);
    QEXPECT_FAIL("tag - both", "Cannot set both ID and RID context", Continue);
    QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
    QCOMPARE(out.contextId(ScopeContext::Tag), tagId);
    QCOMPARE(out.hasContextRID(ScopeContext::Tag), hasTagRid);
    QCOMPARE(out.contextRID(ScopeContext::Tag), tagRid);

    if (hasColId || hasColRid) {
        ScopeContext clear = out;
        clear.clearContext(ScopeContext::Collection);
        QCOMPARE(clear.hasContextId(ScopeContext::Collection), false);
        QCOMPARE(clear.hasContextRID(ScopeContext::Collection), false);
        QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
        QCOMPARE(clear.hasContextId(ScopeContext::Tag), hasTagId);
        QCOMPARE(clear.hasContextRID(ScopeContext::Tag), hasTagRid);
    }
    if (hasTagId || hasTagRid) {
        ScopeContext clear = out;
        clear.clearContext(ScopeContext::Tag);
        QEXPECT_FAIL("both - both", "Cannot set both ID and RID context", Continue);
        QCOMPARE(clear.hasContextId(ScopeContext::Collection), hasColId);
        QCOMPARE(clear.hasContextRID(ScopeContext::Collection), hasColRid);
        QCOMPARE(clear.hasContextId(ScopeContext::Tag), false);
        QCOMPARE(clear.hasContextRID(ScopeContext::Tag), false);
    }

    out.clearContext(ScopeContext::Collection);
    out.clearContext(ScopeContext::Tag);
    QVERIFY(out.isEmpty());
}

void ProtocolTest::testPartMetaData()
{
    PartMetaData in;
    in.setName("PLD:HEAD");
    in.setSize(42);
    in.setVersion(1);
    in.setStorageType(PartMetaData::External);

    const PartMetaData out = serializeAndDeserialize(in);
    QCOMPARE(out.name(), QByteArray("PLD:HEAD"));
    QCOMPARE(out.size(), 42);
    QCOMPARE(out.version(), 1);
    QCOMPARE(out.storageType(), PartMetaData::External);
    QCOMPARE(out, in);
    const bool notEquals = (in != out);
    QVERIFY(!notEquals);
}

void ProtocolTest::testCachePolicy()
{
    CachePolicy in;
    in.setInherit(true);
    in.setCheckInterval(42);
    in.setCacheTimeout(10);
    in.setSyncOnDemand(true);
    in.setLocalParts({ QStringLiteral("PLD:HEAD"), QStringLiteral("PLD:ENVELOPE") });

    const CachePolicy out = serializeAndDeserialize(in);
    QCOMPARE(out.inherit(), true);
    QCOMPARE(out.checkInterval(), 42);
    QCOMPARE(out.cacheTimeout(), 10);
    QCOMPARE(out.syncOnDemand(), true);
    QCOMPARE(out.localParts(), QStringList() << QStringLiteral("PLD:HEAD") << QStringLiteral("PLD:ENVELOPE"));
    QCOMPARE(out, in);
    const bool notEquals = (out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testHelloResponse()
{
    HelloResponse in;
    QVERIFY(in.isResponse());
    QVERIFY(in.isValid());
    QVERIFY(!in.isError());
    in.setServerName(QStringLiteral("AkonadiTest"));
    in.setMessage(QStringLiteral("Oh, hello there!"));
    in.setProtocolVersion(42);
    in.setError(10, QStringLiteral("Ooops"));

    const auto out = serializeAndDeserialize(HelloResponsePtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(out->isResponse());
    QVERIFY(out->isError());
    QCOMPARE(out->errorCode(), 10);
    QCOMPARE(out->errorMessage(), QStringLiteral("Ooops"));
    QCOMPARE(out->serverName(), QStringLiteral("AkonadiTest"));
    QCOMPARE(out->message(), QStringLiteral("Oh, hello there!"));
    QCOMPARE(out->protocolVersion(), 42);
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testLoginCommand()
{
    LoginCommand in;
    QVERIFY(!in.isResponse());
    QVERIFY(in.isValid());
    in.setSessionId("MySession-123-notifications");

    const auto out = serializeAndDeserialize(LoginCommandPtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(!out->isResponse());
    QCOMPARE(out->sessionId(), QByteArray("MySession-123-notifications"));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testLoginResponse()
{
    LoginResponse in;
    QVERIFY(in.isResponse());
    QVERIFY(in.isValid());
    QVERIFY(!in.isError());
    in.setError(42, QStringLiteral("Ooops"));

    const auto out = serializeAndDeserialize(LoginResponsePtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(out->isResponse());
    QVERIFY(out->isError());
    QCOMPARE(out->errorCode(), 42);
    QCOMPARE(out->errorMessage(), QStringLiteral("Ooops"));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testLogoutCommand()
{
    LogoutCommand in;
    QVERIFY(!in.isResponse());
    QVERIFY(in.isValid());

    const auto out = serializeAndDeserialize(LogoutCommandPtr::create(in));
    QVERIFY(!out->isResponse());
    QVERIFY(out->isValid());
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testLogoutResponse()
{
    LogoutResponse in;
    QVERIFY(in.isResponse());
    QVERIFY(in.isValid());
    QVERIFY(!in.isError());
    in.setError(42, QStringLiteral("Ooops"));

    const auto out = serializeAndDeserialize(LogoutResponsePtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(out->isResponse());
    QVERIFY(out->isError());
    QCOMPARE(out->errorCode(), 42);
    QCOMPARE(out->errorMessage(), QStringLiteral("Ooops"));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}


void ProtocolTest::testTransactionCommand()
{
    TransactionCommand in;
    QVERIFY(!in.isResponse());
    QVERIFY(in.isValid());
    in.setMode(TransactionCommand::Begin);

    const auto out = serializeAndDeserialize(TransactionCommandPtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(!out->isResponse());
    QCOMPARE(out->mode(), TransactionCommand::Begin);
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testTransactionResponse()
{
    TransactionResponse in;
    QVERIFY(in.isResponse());
    QVERIFY(in.isValid());
    QVERIFY(!in.isError());
    in.setError(42, QStringLiteral("Ooops"));

    const auto out = serializeAndDeserialize(TransactionResponsePtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(out->isResponse());
    QVERIFY(out->isError());
    QCOMPARE(out->errorCode(), 42);
    QCOMPARE(out->errorMessage(), QStringLiteral("Ooops"));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testCreateItemCommand()
{
    Scope addedTags(QVector<qint64>{ 3, 4 });
    Scope removedTags(QVector<qint64>{ 5, 6 });
    Attributes attrs{ { "ATTR1", "MyAttr" }, { "ATTR2", "Můj chlupaťoučký kůň" } };
    QSet<QByteArray> parts{ "PLD:HEAD", "PLD:ENVELOPE" };

    CreateItemCommand in;
    QVERIFY(!in.isResponse());
    QVERIFY(in.isValid());
    QCOMPARE(in.mergeModes(), CreateItemCommand::None);
    in.setMergeModes(CreateItemCommand::MergeModes(CreateItemCommand::GID | CreateItemCommand::RemoteID));
    in.setCollection(Scope(1));
    in.setItemSize(100);
    in.setMimeType(QStringLiteral("text/directory"));
    in.setGid(QStringLiteral("GID"));
    in.setRemoteId(QStringLiteral("RID"));
    in.setRemoteRevision(QStringLiteral("RREV"));
    in.setDateTime(QDateTime(QDate(2015, 8, 11), QTime(14, 32, 21), Qt::UTC));
    in.setFlags({ "\\SEEN", "FLAG" });
    in.setFlagsOverwritten(true);
    in.setAddedFlags({ "FLAG2" });
    in.setRemovedFlags({ "FLAG3" });
    in.setTags(Scope(2));
    in.setAddedTags(addedTags);
    in.setRemovedTags(removedTags);
    in.setAttributes(attrs);
    in.setParts(parts);

    const auto out = serializeAndDeserialize(CreateItemCommandPtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(!out->isResponse());
    QCOMPARE(out->mergeModes(), CreateItemCommand::GID | CreateItemCommand::RemoteID);
    QCOMPARE(out->collection(), Scope(1));
    QCOMPARE(out->itemSize(), 100);
    QCOMPARE(out->mimeType(), QStringLiteral("text/directory"));
    QCOMPARE(out->gid(), QStringLiteral("GID"));
    QCOMPARE(out->remoteId(), QStringLiteral("RID"));
    QCOMPARE(out->remoteRevision(), QStringLiteral("RREV"));
    QCOMPARE(out->dateTime(), QDateTime(QDate(2015, 8, 11), QTime(14, 32, 21), Qt::UTC));
    QCOMPARE(out->flags(), QSet<QByteArray>() << "\\SEEN" << "FLAG");
    QCOMPARE(out->flagsOverwritten(), true);
    QCOMPARE(out->addedFlags(), QSet<QByteArray>{ "FLAG2" });
    QCOMPARE(out->removedFlags(), QSet<QByteArray>{ "FLAG3" });
    QCOMPARE(out->tags(), Scope(2));
    QCOMPARE(out->addedTags(), addedTags);
    QCOMPARE(out->removedTags(), removedTags);
    QCOMPARE(out->attributes(), attrs);
    QCOMPARE(out->parts(), parts);
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testCreateItemResponse()
{
    CreateItemResponse in;
    QVERIFY(in.isResponse());
    QVERIFY(in.isValid());
    QVERIFY(!in.isError());
    in.setError(42, QStringLiteral("Ooops"));

    const auto out = serializeAndDeserialize(CreateItemResponsePtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(out->isResponse());
    QVERIFY(out->isError());
    QCOMPARE(out->errorCode(), 42);
    QCOMPARE(out->errorMessage(), QStringLiteral("Ooops"));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testCopyItemsCommand()
{
    const Scope items(QVector<qint64>{ 1, 2, 3, 10 });

    CopyItemsCommand in;
    QVERIFY(in.isValid());
    QVERIFY(!in.isResponse());
    in.setItems(items);
    in.setDestination(Scope(42));

    const auto out = serializeAndDeserialize(CopyItemsCommandPtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(!out->isResponse());
    QCOMPARE(out->items(), items);
    QCOMPARE(out->destination(), Scope(42));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

void ProtocolTest::testCopyItemsResponse()
{
    CopyItemsResponse in;
    QVERIFY(in.isResponse());
    QVERIFY(in.isValid());
    QVERIFY(!in.isError());
    in.setError(42, QStringLiteral("Ooops"));

    const auto out = serializeAndDeserialize(CopyItemsResponsePtr::create(in));
    QVERIFY(out->isValid());
    QVERIFY(out->isResponse());
    QVERIFY(out->isError());
    QCOMPARE(out->errorCode(), 42);
    QCOMPARE(out->errorMessage(), QStringLiteral("Ooops"));
    QCOMPARE(*out, in);
    const bool notEquals = (*out != in);
    QVERIFY(!notEquals);
}

QTEST_MAIN(ProtocolTest)
