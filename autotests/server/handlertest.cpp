/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <aktest.h>
#include <private/protocol_p.h>

#include <typeinfo>

#include "handler.h"
#include "handler/collectioncreatehandler.h"
#include "handler/collectionfetchhandler.h"
#include "handler/collectionstatsfetchhandler.h"
#include "handler/collectiondeletehandler.h"
#include "handler/collectionmodifyhandler.h"
#include "handler/collectioncopyhandler.h"
#include "handler/collectionmovehandler.h"
#include "handler/searchcreatehandler.h"
#include "handler/searchhandler.h"
#include "handler/itemfetchhandler.h"
#include "handler/itemdeletehandler.h"
#include "handler/itemmodifyhandler.h"
#include "handler/itemcreatehandler.h"
#include "handler/itemcopyhandler.h"
#include "handler/itemlinkhandler.h"
#include "handler/itemmovehandler.h"
#include "handler/resourceselecthandler.h"
#include "handler/transactionhandler.h"
#include "handler/loginhandler.h"
#include "handler/logouthandler.h"

#include "fakeakonadiserver.h"

using namespace Akonadi;
using namespace Akonadi::Server;

#define MAKE_CMD_ROW( command, class ) QTest::newRow(#command) << (command) << QByteArray(typeid(Akonadi::Server::class).name());

class HandlerTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;
public:
    explicit HandlerTest()
    {
        mAkonadi.init();
    }

private:
    void setupTestData()
    {
        QTest::addColumn<Protocol::Command::Type>("command");
        QTest::addColumn<QByteArray>("className");
    }

    void addAuthCommands()
    {
        MAKE_CMD_ROW(Protocol::Command::CreateCollection, CollectionCreateHandler)
        MAKE_CMD_ROW(Protocol::Command::FetchCollections, CollectionFetchHandler)
        MAKE_CMD_ROW(Protocol::Command::StoreSearch, SearchCreateHandler)
        MAKE_CMD_ROW(Protocol::Command::Search, SearchHandler)
        MAKE_CMD_ROW(Protocol::Command::FetchItems, ItemFetchHandler)
        MAKE_CMD_ROW(Protocol::Command::ModifyItems, ItemModifyHandler)
        MAKE_CMD_ROW(Protocol::Command::FetchCollectionStats, CollectionStatsFetchHandler)
        MAKE_CMD_ROW(Protocol::Command::DeleteCollection, CollectionDeleteHandler)
        MAKE_CMD_ROW(Protocol::Command::ModifyCollection, CollectionModifyHandler)
        MAKE_CMD_ROW(Protocol::Command::Transaction, TransactionHandler)
        MAKE_CMD_ROW(Protocol::Command::CreateItem, ItemCreateHandler)
        MAKE_CMD_ROW(Protocol::Command::CopyItems, ItemCopyHandler)
        MAKE_CMD_ROW(Protocol::Command::CopyCollection, CollectionCopyHandler)
        MAKE_CMD_ROW(Protocol::Command::LinkItems, ItemLinkHandler)
        MAKE_CMD_ROW(Protocol::Command::SelectResource, ResourceSelectHandler)
        MAKE_CMD_ROW(Protocol::Command::DeleteItems, ItemDeleteHandler)
        MAKE_CMD_ROW(Protocol::Command::MoveItems, ItemMoveHandler)
        MAKE_CMD_ROW(Protocol::Command::MoveCollection, CollectionMoveHandler)
    }

    void addNonAuthCommands()
    {
        MAKE_CMD_ROW(Protocol::Command::Login, LoginHandler)
    }

    void addAlwaysCommands()
    {
        MAKE_CMD_ROW(Protocol::Command::Logout, LogoutHandler)
    }

    void addInvalidCommands()
    {
        //MAKE_CMD_ROW(Protocol::Command::Invalid, UnknownCommandHandler)
    }

    template<typename T>
    QByteArray typeName(const T &t)
    {
        const auto &v = *t;
        return typeid(v).name();
    }


private Q_SLOTS:
    void testFindAuthenticatedCommand_data()
    {
        setupTestData();
        addAuthCommands();
    }

    void testFindAuthenticatedCommand()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QByteArray, className);
        const auto handler = Handler::findHandlerForCommandAuthenticated(command, mAkonadi);
        QVERIFY(handler);
        QCOMPARE(typeName(handler), className);
    }

    void testFindAuthenticatedCommandNegative_data()
    {
        setupTestData();
        addNonAuthCommands();
        addAlwaysCommands();
        addInvalidCommands();
    }

    void testFindAuthenticatedCommandNegative()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QByteArray, className);

        const auto handler = Handler::findHandlerForCommandAuthenticated(command, mAkonadi);
        QVERIFY(!handler);
    }

    void testFindNonAutenticatedCommand_data()
    {
        setupTestData();
        addNonAuthCommands();
    }

    void testFindNonAutenticatedCommand()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QByteArray, className);


        auto handler = Handler::findHandlerForCommandNonAuthenticated(command, mAkonadi);
        QVERIFY(handler);
        QCOMPARE(typeName(handler), className);
    }

    void testFindAlwaysCommand_data()
    {
        setupTestData();
        addAlwaysCommands();
    }

    void testFindAlwaysCommand()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QByteArray, className);

        const auto handler = Handler::findHandlerForCommandAlwaysAllowed(command, mAkonadi);
        QVERIFY(handler);
        QCOMPARE(typeName(handler), className);
    }

    void testFindAlwaysCommandNegative_data()
    {
        setupTestData();
        addAuthCommands();
        addNonAuthCommands();
        addInvalidCommands();
    }

    void testFindAlwaysCommandNegative()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QByteArray, className);

        const auto handler = Handler::findHandlerForCommandAlwaysAllowed(command, mAkonadi);
        QVERIFY(!handler);
    }
};

AKTEST_MAIN(HandlerTest)

#include "handlertest.moc"
