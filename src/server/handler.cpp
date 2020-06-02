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

#include "handler/collectioncopyhandler.h"
#include "handler/collectioncreatehandler.h"
#include "handler/collectiondeletehandler.h"
#include "handler/collectionfetchhandler.h"
#include "handler/collectionmodifyhandler.h"
#include "handler/collectionmovehandler.h"
#include "handler/collectionstatsfetchhandler.h"
#include "handler/itemcopyhandler.h"
#include "handler/itemcreatehandler.h"
#include "handler/itemdeletehandler.h"
#include "handler/itemfetchhandler.h"
#include "handler/itemlinkhandler.h"
#include "handler/itemmodifyhandler.h"
#include "handler/itemmovehandler.h"
#include "handler/loginhandler.h"
#include "handler/logouthandler.h"
#include "handler/relationfetchhandler.h"
#include "handler/relationremovehandler.h"
#include "handler/relationmodifyhandler.h"
#include "handler/resourceselecthandler.h"
#include "handler/searchcreatehandler.h"
#include "handler/searchhandler.h"
#include "handler/searchresulthandler.h"
#include "handler/tagcreatehandler.h"
#include "handler/tagdeletehandler.h"
#include "handler/tagfetchhandler.h"
#include "handler/tagmodifyhandler.h"
#include "handler/transactionhandler.h"
#include "storage/querybuilder.h"

using namespace Akonadi;
using namespace Akonadi::Server;

std::unique_ptr<Handler> Handler::findHandlerForCommandNonAuthenticated(Protocol::Command::Type cmd, AkonadiServer &akonadi)
{
    // allowed are LOGIN
    if (cmd == Protocol::Command::Login) {
        return std::make_unique<LoginHandler>(akonadi);
    }

    return {};
}

std::unique_ptr<Handler> Handler::findHandlerForCommandAlwaysAllowed(Protocol::Command::Type cmd, AkonadiServer &akonadi)
{
    // allowed is LOGOUT
    if (cmd == Protocol::Command::Logout) {
        return std::make_unique<LogoutHandler>(akonadi);
    }
    return nullptr;
}

std::unique_ptr<Handler> Handler::findHandlerForCommandAuthenticated(Protocol::Command::Type cmd, AkonadiServer &akonadi)
{
    switch (cmd) {
    case Protocol::Command::Invalid:
        Q_ASSERT_X(cmd != Protocol::Command::Invalid, __FUNCTION__,
                   "Invalid command is not allowed");
        return {};
    case Protocol::Command::Hello:
        Q_ASSERT_X(cmd != Protocol::Command::Hello, __FUNCTION__,
                   "Hello command is not allowed in this context");
        return {};
    case Protocol::Command::Login:
    case Protocol::Command::Logout:
        return {};
    case Protocol::Command::_ResponseBit:
        Q_ASSERT_X(cmd != Protocol::Command::_ResponseBit, __FUNCTION__,
                   "ResponseBit is not a valid command type");
        return {};

    case Protocol::Command::Transaction:
        return std::make_unique<TransactionHandler>(akonadi);

    case Protocol::Command::CreateItem:
        return std::make_unique<ItemCreateHandler>(akonadi);
    case Protocol::Command::CopyItems:
        return std::make_unique<ItemCopyHandler>(akonadi);
    case Protocol::Command::DeleteItems:
        return std::make_unique<ItemDeleteHandler>(akonadi);
    case Protocol::Command::FetchItems:
        return std::make_unique<ItemFetchHandler>(akonadi);
    case Protocol::Command::LinkItems:
        return std::make_unique<ItemLinkHandler>(akonadi);
    case Protocol::Command::ModifyItems:
        return std::make_unique<ItemModifyHandler>(akonadi);
    case Protocol::Command::MoveItems:
        return std::make_unique<ItemMoveHandler>(akonadi);

    case Protocol::Command::CreateCollection:
        return std::make_unique<CollectionCreateHandler>(akonadi);
    case Protocol::Command::CopyCollection:
        return std::make_unique<CollectionCopyHandler>(akonadi);
    case Protocol::Command::DeleteCollection:
        return std::make_unique<CollectionDeleteHandler>(akonadi);
    case Protocol::Command::FetchCollections:
        return std::make_unique<CollectionFetchHandler>(akonadi);
    case Protocol::Command::FetchCollectionStats:
        return std::make_unique<CollectionStatsFetchHandler>(akonadi);
    case Protocol::Command::ModifyCollection:
        return std::make_unique<CollectionModifyHandler>(akonadi);
    case Protocol::Command::MoveCollection:
        return std::make_unique<CollectionMoveHandler>(akonadi);

    case Protocol::Command::Search:
        return std::make_unique<SearchHandler>(akonadi);
    case Protocol::Command::SearchResult:
        return std::make_unique<SearchResultHandler>(akonadi);
    case Protocol::Command::StoreSearch:
        return std::make_unique<SearchCreateHandler>(akonadi);

    case Protocol::Command::CreateTag:
        return std::make_unique<TagCreateHandler>(akonadi);
    case Protocol::Command::DeleteTag:
        return std::make_unique<TagDeleteHandler>(akonadi);
    case Protocol::Command::FetchTags:
        return std::make_unique<TagFetchHandler>(akonadi);
    case Protocol::Command::ModifyTag:
        return std::make_unique<TagModifyHandler>(akonadi);

    case Protocol::Command::FetchRelations:
        return std::make_unique<RelationFetchHandler>(akonadi);
    case Protocol::Command::ModifyRelation:
        return std::make_unique<RelationModifyHandler>(akonadi);
    case Protocol::Command::RemoveRelations:
        return std::make_unique<RelationRemoveHandler>(akonadi);

    case Protocol::Command::SelectResource:
        return std::make_unique<ResourceSelectHandler>(akonadi);

    case Protocol::Command::StreamPayload:
        Q_ASSERT_X(cmd != Protocol::Command::StreamPayload, __FUNCTION__,
                   "StreamPayload command is not allowed in this context");
        return {};

    case Protocol::Command::ItemChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::ItemChangeNotification, __FUNCTION__,
                   "ItemChangeNotification command is not allowed on this connection");
        return {};
    case Protocol::Command::CollectionChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::CollectionChangeNotification, __FUNCTION__,
                   "CollectionChangeNotification command is not allowed on this connection");
        return {};
    case Protocol::Command::TagChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::TagChangeNotification, __FUNCTION__,
                   "TagChangeNotification command is not allowed on this connection");
        return {};
    case Protocol::Command::RelationChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::RelationChangeNotification, __FUNCTION__,
                   "RelationChangeNotification command is not allowed on this connection");
        return {};
    case Protocol::Command::SubscriptionChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::SubscriptionChangeNotification, __FUNCTION__,
                   "SubscriptionChangeNotification command is not allowed on this connection");
        return {};
    case Protocol::Command::DebugChangeNotification:
        Q_ASSERT_X(cmd != Protocol::Command::DebugChangeNotification, __FUNCTION__,
                   "DebugChangeNotification command is not allowed on this connection");
        return {};
    case Protocol::Command::ModifySubscription:
        Q_ASSERT_X(cmd != Protocol::Command::ModifySubscription, __FUNCTION__,
                   "ModifySubscription command is not allowed on this connection");
        return {};
    case Protocol::Command::CreateSubscription:
        Q_ASSERT_X(cmd != Protocol::Command::CreateSubscription, __FUNCTION__,
                   "CreateSubscription command is not allowed on this connection");
        return {};
    }

    return {};
}

Handler::Handler(AkonadiServer &akonadi)
    : m_akonadi(akonadi)
{}

void Handler::setTag(quint64 tag)
{
    m_tag = tag;
}

quint64 Handler::tag() const
{
    return m_tag;
}

void Handler::setCommand(const Protocol::CommandPtr &cmd)
{
    m_command = cmd;
}

Protocol::CommandPtr Handler::command() const
{
    return m_command;
}

void Handler::setConnection(Connection *connection)
{
    m_connection = connection;
}

Connection *Handler::connection() const
{
    return m_connection;
}

DataStore *Handler::storageBackend() const
{
    return m_connection->storageBackend();
}

AkonadiServer &Handler::akonadi() const
{
    return m_akonadi;
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
        Protocol::ResponsePtr r = Protocol::Factory::response(m_command->type());
        // FIXME: Error enums?
        r->setError(1, failureMessage);

        m_connection->sendResponse(m_tag, r);
    }

    return false;
}

bool Handler::checkScopeConstraints(const Akonadi::Scope &scope, int permittedScopes)
{
    return scope.scope() & permittedScopes;
}
