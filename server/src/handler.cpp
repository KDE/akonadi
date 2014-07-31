/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "handler.h"

#include <QtCore/QDebug>
#include <QtCore/QLatin1String>

#include "libs/imapset_p.h"
#include "libs/protocol_p.h"

#include "connection.h"
#include "response.h"
#include "scope.h"
#include "handler/akappend.h"
#include "handler/append.h"
#include "handler/capability.h"
#include "handler/copy.h"
#include "handler/colcopy.h"
#include "handler/colmove.h"
#include "handler/create.h"
#include "handler/delete.h"
#include "handler/expunge.h"
#include "handler/fetch.h"
#include "handler/link.h"
#include "handler/list.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/merge.h"
#include "handler/modify.h"
#include "handler/move.h"
#include "handler/remove.h"
#include "handler/resourceselect.h"
#include "handler/search.h"
#include "handler/searchpersistent.h"
#include "handler/searchresult.h"
#include "handler/select.h"
#include "handler/subscribe.h"
#include "handler/status.h"
#include "handler/store.h"
#include "handler/transaction.h"
#include "handler/tagappend.h"
#include "handler/tagfetch.h"
#include "handler/tagremove.h"
#include "handler/tagstore.h"

#include "storage/querybuilder.h"
#include "imapstreamparser.h"

using namespace Akonadi::Server;

Handler::Handler()
    : QObject()
    , m_connection(0)
    , m_streamParser(0)
{
}

Handler::~Handler()
{
}

Handler *Handler::findHandlerForCommandNonAuthenticated(const QByteArray &command)
{
    // allowed are LOGIN
    if (command == AKONADI_CMD_LOGIN) {
        return new Login();
    }

    return 0;
}

Handler *Handler::findHandlerForCommandAlwaysAllowed(const QByteArray &command)
{
    // allowed commands CAPABILITY and LOGOUT
    if (command == AKONADI_CMD_LOGOUT) {
        return new Logout();
    }
    if (command == AKONADI_CMD_CAPABILITY) {
        return new Capability();
    }
    return 0;
}

void Handler::setTag(const QByteArray &tag)
{
    m_tag = tag;
}

QByteArray Handler::tag() const
{
    return m_tag;
}

Handler *Handler::findHandlerForCommandAuthenticated(const QByteArray &_command, ImapStreamParser *streamParser)
{
    QByteArray command(_command);
    // deal with command prefixes
    Scope::SelectionScope scope = Scope::selectionScopeFromByteArray(command);
    if (scope != Scope::None) {
        command = streamParser->readString();
    }

    // allowed commands are listed below ;-).
    if (command == AKONADI_CMD_APPEND) {
        return new Append();
    }
    if (command == AKONADI_CMD_COLLECTIONCREATE) {
        return new Create(scope);
    }
    if (command == AKONADI_CMD_LIST || command == AKONADI_CMD_X_AKLIST) {   //TODO: remove X-AKLIST support in Akonadi 2.0
        return new List(scope, false);
    }
    if (command == AKONADI_CMD_LSUB || command == AKONADI_CMD_X_AKLSUB) {   //TODO: remove X-AKLSUB support in Akonadi 2.0
        return new List(scope, true);
    }
    if (command == AKONADI_CMD_SELECT) {
        return new Select(scope);
    }
    if (command == AKONADI_CMD_SEARCH_STORE) {
        return new SearchPersistent();
    }
    if (command == AKONADI_CMD_SEARCH) {
        return new Search();
    }
    if (command == AKONADI_CMD_SEARCH_RESULT) {
        return new SearchResult(scope);
    }
    if (command == AKONADI_CMD_ITEMFETCH) {
        return new Fetch(scope);
    }
    if (command == AKONADI_CMD_EXPUNGE) {   //TODO: remove EXPUNGE support in Akonadi 2.0
        return new Expunge();
    }
    if (command == AKONADI_CMD_ITEMMODIFY) {
        return new Store(scope);
    }
    if (command == AKONADI_CMD_STATUS) {
        return new Status();
    }
    if (command == AKONADI_CMD_COLLECTIONDELETE) {
        return new Delete(scope);
    }
    if (command == AKONADI_CMD_COLLECTIONMODIFY) {
        return new Modify(scope);
    }
    if (command == AKONADI_CMD_BEGIN) {
        return new TransactionHandler(TransactionHandler::Begin);
    }
    if (command == AKONADI_CMD_ROLLBACK) {
        return new TransactionHandler(TransactionHandler::Rollback);
    }
    if (command == AKONADI_CMD_COMMIT) {
        return new TransactionHandler(TransactionHandler::Commit);
    }
    if (command == AKONADI_CMD_ITEMCREATE) {
        return new AkAppend();
    }
    if (command == AKONADI_CMD_SUBSCRIBE) {
        return new Subscribe(true);
    }
    if (command == AKONADI_CMD_UNSUBSCRIBE) {
        return new Subscribe(false);
    }
    if (command == AKONADI_CMD_ITEMCOPY) {
        return new Copy();
    }
    if (command == AKONADI_CMD_COLLECTIONCOPY) {
        return new ColCopy();
    }
    if (command == AKONADI_CMD_ITEMLINK) {
        return new Link(scope, true);
    }
    if (command == AKONADI_CMD_ITEMUNLINK) {
        return new Link(scope, false);
    }
    if (command == AKONADI_CMD_RESOURCESELECT) {
        return new ResourceSelect();
    }
    if (command == AKONADI_CMD_ITEMDELETE) {
        return new Remove(scope);
    }
    if (command == AKONADI_CMD_ITEMMOVE) {
        return new Move(scope);
    }
    if (command == AKONADI_CMD_COLLECTIONMOVE) {
        return new ColMove(scope);
    }
    if (command == AKONADI_CMD_TAGAPPEND) {
        return new TagAppend();
    }
    if (command == AKONADI_CMD_TAGFETCH) {
        return new TagFetch(scope);
    }
    if (command == AKONADI_CMD_TAGREMOVE) {
        return new TagRemove(scope);
    }
    if (command == AKONADI_CMD_TAGSTORE) {
        return new TagStore();
    }
    if (command == AKONADI_CMD_MERGE) {
        return new Merge();
    }

    return 0;
}

void Handler::setConnection(Connection *connection)
{
    m_connection = connection;
}

Connection *Handler::connection() const
{
    return m_connection;
}

bool Handler::failureResponse(const QByteArray &failureMessage)
{
    Response response;
    response.setTag(tag());
    response.setFailure();
    response.setString(failureMessage);
    Q_EMIT responseAvailable(response);
    return false;
}

bool Handler::failureResponse(const char *failureMessage)
{
    return failureResponse(QByteArray(failureMessage));
}

bool Handler::successResponse(const QByteArray &successMessage)
{
    Response response;
    response.setTag(tag());
    response.setSuccess();
    response.setString(successMessage);
    Q_EMIT responseAvailable(response);
    return true;
}

bool Handler::successResponse(const char *successMessage)
{
    return successResponse(QByteArray(successMessage));
}

void Handler::setStreamParser(ImapStreamParser *parser)
{
    m_streamParser = parser;
}

UnknownCommandHandler::UnknownCommandHandler(const QByteArray &command)
    : mCommand(command)
{
}

bool UnknownCommandHandler::parseStream()
{
    Response response;
    response.setError();
    response.setTag(tag());
    if (mCommand.isEmpty()) {
        response.setString("No command specified");
    } else {
        response.setString("Unrecognized command: " + mCommand);
    }
    m_streamParser->readUntilCommandEnd();

    Q_EMIT responseAvailable(response);
    return true;
}
