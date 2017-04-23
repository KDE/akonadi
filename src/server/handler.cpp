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

#include <private/scope_p.h>
#include <private/protocol_p.h>

#include "connection.h"

#include "handler/akappend.h"
#include "handler/copy.h"
#include "handler/colcopy.h"
#include "handler/colmove.h"
#include "handler/create.h"
#include "handler/delete.h"
#include "handler/fetch.h"
#include "handler/link.h"
#include "handler/list.h"
#include "handler/login.h"
#include "handler/logout.h"
#include "handler/modify.h"
#include "handler/move.h"
#include "handler/remove.h"
#include "handler/resourceselect.h"
#include "handler/search.h"
#include "handler/searchpersistent.h"
#include "handler/searchresult.h"
#include "handler/status.h"
#include "handler/store.h"
#include "handler/transaction.h"
#include "handler/tagappend.h"
#include "handler/tagfetch.h"
#include "handler/tagremove.h"
#include "handler/tagstore.h"
#include "handler/relationstore.h"
#include "handler/relationremove.h"
#include "handler/relationfetch.h"

#include "storage/querybuilder.h"

using namespace Akonadi;
using namespace Akonadi::Server;

Handler::Handler()
    : QObject()
    , m_connection(nullptr)
    , m_sentFailureResponse(false)
{
}

Handler::~Handler()
{
}

Handler *Handler::findHandlerForCommandNonAuthenticated(Protocol::Command::Type cmd)
{
    // allowed are LOGIN
    if (cmd == Protocol::Command::Login) {
        return new Login();
    }

    return nullptr;
}

Handler *Handler::findHandlerForCommandAlwaysAllowed(Protocol::Command::Type cmd)
{
    // allowed is LOGOUT
    if (cmd == Protocol::Command::Logout) {
        return new Logout();
    }
    return nullptr;
}

void Handler::setTag(quint64 tag)
{
    m_tag = tag;
}

quint64 Handler::tag() const
{
    return m_tag;
}

void Handler::setCommand(const Protocol::Command &cmd)
{
    m_command = cmd;
}

Protocol::Command Handler::command() const
{
    return m_command;
}

Handler *Handler::findHandlerForCommandAuthenticated(Protocol::Command::Type cmd)
{
    switch (cmd) {
    case Protocol::Command::Invalid:
        Q_ASSERT_X(cmd != Protocol::Command::Invalid, __FUNCTION__,
                   "Invalid command is not allowed");
        return nullptr;
    case Protocol::Command::Hello:
        Q_ASSERT_X(cmd != Protocol::Command::Hello, __FUNCTION__,
                   "Hello command is not allowed in this context");
        return nullptr;
    case Protocol::Command::Login:
        Q_ASSERT_X(cmd != Protocol::Command::StreamPayload, __FUNCTION__,
                   "Login command is not allowed in this context");
        return nullptr;
    case Protocol::Command::Logout:
        Q_ASSERT_X(cmd != Protocol::Command::StreamPayload, __FUNCTION__,
                   "Logout command is not allowed in this context");
        return nullptr;
    case Protocol::Command::_ResponseBit:
        Q_ASSERT_X(cmd != Protocol::Command::_ResponseBit, __FUNCTION__,
                   "ResponseBit is not a valid command type");
        return nullptr;

    case Protocol::Command::Transaction:
        return new TransactionHandler();

    case Protocol::Command::CreateItem:
        return new AkAppend();
    case Protocol::Command::CopyItems:
        return new Copy();
    case Protocol::Command::DeleteItems:
        return new Remove();
    case Protocol::Command::FetchItems:
        return new Fetch();
    case Protocol::Command::LinkItems:
        return new Link();
    case Protocol::Command::ModifyItems:
        return new Store();
    case Protocol::Command::MoveItems:
        return new Move();

    case Protocol::Command::CreateCollection:
        return new Create();
    case Protocol::Command::CopyCollection:
        return new ColCopy();
    case Protocol::Command::DeleteCollection:
        return new Delete();
    case Protocol::Command::FetchCollections:
        return new List();
    case Protocol::Command::FetchCollectionStats:
        return new Status();
    case Protocol::Command::ModifyCollection:
        return new Modify();
    case Protocol::Command::MoveCollection:
        return new ColMove();

    case Protocol::Command::Search:
        return new Search();
    case Protocol::Command::SearchResult:
        return new SearchResult();
    case Protocol::Command::StoreSearch:
        return new SearchPersistent();

    case Protocol::Command::CreateTag:
        return new TagAppend();
    case Protocol::Command::DeleteTag:
        return new TagRemove();
    case Protocol::Command::FetchTags:
        return new TagFetch();
    case Protocol::Command::ModifyTag:
        return new TagStore();

    case Protocol::Command::FetchRelations:
        return new RelationFetch();
    case Protocol::Command::ModifyRelation:
        return new RelationStore();
    case Protocol::Command::RemoveRelations:
        return new RelationRemove();

    case Protocol::Command::SelectResource:
        return new ResourceSelect();

    case Protocol::Command::StreamPayload:
        Q_ASSERT_X(cmd != Protocol::Command::StreamPayload, __FUNCTION__,
                   "StreamPayload command is not allowed in this context");
        return nullptr;

    case Protocol::Command::ItemChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::ItemChangeNotification, __FUNCTION__,
                   "ItemChangeNotification command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::CollectionChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::CollectionChangeNotification, __FUNCTION__,
                   "CollectionChangeNotification command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::TagChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::TagChangeNotification, __FUNCTION__,
                   "TagChangeNotification command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::RelationChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::RelationChangeNotification, __FUNCTION__,
                   "RelationChangeNotification command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::SubscriptionChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::SubscriptionChangeNotification, __FUNCTION__,
                   "SubscriptionChangeNotification command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::DebugChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::DebugChangeNotification, __FUNCTION__,
                   "DebugChangeNotification command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::ModifySubscription:
        Q_ASSERT_X(cmd != Protocol::Command::ModifySubscription, __FUNCTION__,
                   "ModifySubscription command is not allowed on this connection");
        return nullptr;
    case Protocol::Command::CreateSubscription:
        Q_ASSERT_X(cmd != Protocol::Command::CreateSubscription, __FUNCTION__,
                   "CreateSubscription command is not allowed on this connection");
        return nullptr;
    }

    return nullptr;
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
    return failureResponse(QString::fromUtf8(failureMessage));
}

bool Handler::failureResponse(const char *failureMessage)
{
    return failureResponse(QString::fromUtf8(failureMessage));
}

bool Handler::failureResponse(const QString &failureMessage)
{
    // Prevent sending multiple error responses from a single handler (or from
    // a handler and then from Connection, since clients only expect a single
    // error response
    if (!m_sentFailureResponse) {
        m_sentFailureResponse = true;
        Protocol::Response r = Protocol::Factory::response(m_command.type());
        // FIXME: Error enums?
        r.setError(1, failureMessage);

        sendResponse(r);
    }

    return false;
}

void Handler::sendResponse(const Protocol::Command &response)
{
    m_connection->sendResponse(response);
}

bool Handler::checkScopeConstraints(const Akonadi::Scope &scope, int permittedScopes)
{
    return scope.scope() & permittedScopes;
}
