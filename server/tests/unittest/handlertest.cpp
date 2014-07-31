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
#include <QtTest/QTest>

#include <aktest.h>

#include "handler.h"

using namespace Akonadi::Server;

#define MAKE_CMD_ROW( command, class ) QTest::newRow(#command) << #command << "Akonadi::Server::" #class;

class HandlerTest : public QObject
{
    Q_OBJECT
private:
    void setupTestData()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("className");
    }

    void addAuthCommands()
    {
        MAKE_CMD_ROW(APPEND, Append)
        MAKE_CMD_ROW(CREATE, Create)
        MAKE_CMD_ROW(LIST, List)
        MAKE_CMD_ROW(LSUB, List)
        MAKE_CMD_ROW(SELECT, Select)
        MAKE_CMD_ROW(SEARCH_STORE, SearchPersistent)
        MAKE_CMD_ROW(SEARCH, Search)
        MAKE_CMD_ROW(FETCH, Fetch)
        MAKE_CMD_ROW(EXPUNGE, Expunge)
        MAKE_CMD_ROW(STORE, Store)
        MAKE_CMD_ROW(STATUS, Status)
        MAKE_CMD_ROW(DELETE, Delete)
        MAKE_CMD_ROW(MODIFY, Modify)
        MAKE_CMD_ROW(BEGIN, TransactionHandler)
        MAKE_CMD_ROW(ROLLBACK, TransactionHandler)
        MAKE_CMD_ROW(COMMIT, TransactionHandler)
        MAKE_CMD_ROW(X - AKAPPEND, AkAppend)
        MAKE_CMD_ROW(SUBSCRIBE, Subscribe)
        MAKE_CMD_ROW(UNSUBSCRIBE, Subscribe)
        MAKE_CMD_ROW(COPY, Copy)
        MAKE_CMD_ROW(COLCOPY, ColCopy)
        MAKE_CMD_ROW(LINK, Link)
        MAKE_CMD_ROW(UNLINK, Link)
        MAKE_CMD_ROW(RESSELECT, ResourceSelect)
        MAKE_CMD_ROW(REMOVE, Remove)
        MAKE_CMD_ROW(MOVE, Move)
        MAKE_CMD_ROW(COLMOVE, ColMove)
    }

    void addNonAuthCommands()
    {
        MAKE_CMD_ROW(LOGIN, Login)
    }

    void addAlwaysCommands()
    {
        MAKE_CMD_ROW(LOGOUT, Logout)
        MAKE_CMD_ROW(CAPABILITY, Capability)
    }

    void addInvalidCommands()
    {
        MAKE_CMD_ROW(UNKNOWN, UnknownCommandHandler)
    }
private Q_SLOTS:
    void testFindAuthenticatedCommand_data()
    {
        setupTestData();
        addAuthCommands();
    }

    void testFindAuthenticatedCommand()
    {
        QFETCH(QString, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAuthenticated(command.toLatin1(), 0));
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
        QFETCH(QString, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAuthenticated(command.toLatin1(), 0));
        QVERIFY(handler.isNull());
    }

    void testFindNonAutenticatedCommand_data()
    {
        setupTestData();
        addNonAuthCommands();
    }

    void testFindNonAutenticatedCommand()
    {
        QFETCH(QString, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandNonAuthenticated(command.toLatin1()));
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
        QFETCH(QString, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandNonAuthenticated(command.toLatin1()));
        QVERIFY(handler.isNull());
    }

    void testFindAlwaysCommand_data()
    {
        setupTestData();
        addAlwaysCommands();
    }

    void testFindAlwaysCommand()
    {
        QFETCH(QString, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAlwaysAllowed(command.toLatin1()));
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
        QFETCH(QString, command);
        QFETCH(QString, className);

        QScopedPointer<Handler> handler(Handler::findHandlerForCommandAlwaysAllowed(command.toLatin1()));
        QVERIFY(handler.isNull());
    }
};

AKTEST_MAIN(HandlerTest)

#include "handlertest.moc"
