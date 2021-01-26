/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "loginhandler.h"

#include "connection.h"

using namespace Akonadi;
using namespace Akonadi::Server;

LoginHandler::LoginHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool LoginHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::LoginCommand>(m_command);

    if (cmd.sessionId().isEmpty()) {
        return failureResponse(QStringLiteral("Missing session identifier"));
    }

    connection()->setSessionId(cmd.sessionId());
    connection()->setState(Server::Authenticated);

    return successResponse<Protocol::LoginResponse>();
}
