/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include <QObject>
#include <QTest>

#include <aktest.h>
#include <private/protocol_p.h>

#include "handler.h"

using namespace Akonadi;
using namespace Akonadi::Server;

#define MAKE_CMD_ROW( command, class ) QTest::newRow(#command) << command << "Akonadi::Server::" #class;

class HandlerTest : public QObject
{
    Q_OBJECT
private:
    void setupTestData()
    {
        QTest::addColumn<Protocol::Command::Type>("command");
        QTest::addColumn<QString>("className");
    }

    void addAuthCommands()
    {
        MAKE_CMD_ROW(Protocol::Command::CreateCollection, Create)
        MAKE_CMD_ROW(Protocol::Command::FetchCollections, List)
        MAKE_CMD_ROW(Protocol::Command::StoreSearch, SearchPersistent)
        MAKE_CMD_ROW(Protocol::Command::Search, Search)
        MAKE_CMD_ROW(Protocol::Command::FetchItems, Fetch)
        MAKE_CMD_ROW(Protocol::Command::ModifyItems, Store)
        MAKE_CMD_ROW(Protocol::Command::FetchCollectionStats, Status)
        MAKE_CMD_ROW(Protocol::Command::DeleteCollection, Delete)
        MAKE_CMD_ROW(Protocol::Command::ModifyCollection, Modify)
        MAKE_CMD_ROW(Protocol::Command::Transaction, TransactionHandler)
        MAKE_CMD_ROW(Protocol::Command::CreateItem, AkAppend)
        MAKE_CMD_ROW(Protocol::Command::CopyItems, Copy)
        MAKE_CMD_ROW(Protocol::Command::CopyCollection, ColCopy)
        MAKE_CMD_ROW(Protocol::Command::LinkItems, Link)
        MAKE_CMD_ROW(Protocol::Command::SelectResource, ResourceSelect)
        MAKE_CMD_ROW(Protocol::Command::DeleteItems, Remove)
        MAKE_CMD_ROW(Protocol::Command::MoveItems, Move)
        MAKE_CMD_ROW(Protocol::Command::MoveCollection, ColMove)
    }

    void addNonAuthCommands()
    {
        MAKE_CMD_ROW(Protocol::Command::Login, Login)
    }

    void addAlwaysCommands()
    {
        MAKE_CMD_ROW(Protocol::Command::Logout, Logout)
    }

    void addInvalidCommands()
    {
        //MAKE_CMD_ROW(Protocol::Command::Invalid, UnknownCommandHandler)
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
        QFETCH(QString, className);
        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAuthenticated(command));
        QVERIFY(!handler.isNull());
        QCOMPARE(handler->metaObject()->className(), className.toLatin1().constData());
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
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAuthenticated(command));
        QVERIFY(handler.isNull());
    }

    void testFindNonAutenticatedCommand_data()
    {
        setupTestData();
        addNonAuthCommands();
    }

    void testFindNonAutenticatedCommand()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandNonAuthenticated(command));
        QVERIFY(!handler.isNull());
        QCOMPARE(handler->metaObject()->className(), className.toLatin1().constData());
    }

    void testFindNonAutenticatedCommandNegative_data()
    {
        setupTestData();
        addAuthCommands();
        addAlwaysCommands();
        addInvalidCommands();
    }

    void testFindNonAutenticatedCommandNegative()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandNonAuthenticated(command));
        QVERIFY(handler.isNull());
    }

    void testFindAlwaysCommand_data()
    {
        setupTestData();
        addAlwaysCommands();
    }

    void testFindAlwaysCommand()
    {
        QFETCH(Protocol::Command::Type, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAlwaysAllowed(command));
        QVERIFY(!handler.isNull());
        QCOMPARE(handler->metaObject()->className(), className.toLatin1().constData());
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
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAlwaysAllowed(command));
        QVERIFY(handler.isNull());
    }
};

AKTEST_MAIN(HandlerTest)

#include "handlertest.moc"
