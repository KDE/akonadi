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
{}

Cookie LoginHandler::getCookie(const Protocol::LoginCommand &cmd) const
{
    // no cookie?
    if (cmd.cookie().isEmpty()) {
        return akonadi().cookieManager().generateCookie();
    }

    return cmd.cookie();
}

bool LoginHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::LoginCommand>(m_command);

    if (cmd.sessionId().isEmpty()) {
        return failureResponse(QStringLiteral("Missing session identifier"));
    }
    connection()->setSessionId(cmd.sessionId());

    const auto cookie = getCookie(cmd);
    auto &cookieData = akonadi().cookieManager().data(cookie);

    std::optional<QString> resourceId;
    resourceId = cookieData.resourceId;

    if (!cmd.resourceId().isEmpty()) {
        resourceId = cmd.resourceId();
    }

    if (resourceId.has_value()) {
        const auto resource = Resource::retrieveByName(*resourceId);
        if (!resource.isValid()) {
            return failureResponse(QStringLiteral("%1 is not a valid resource identifier").arg(*resourceId));
        }

        auto context = connection()->context();
        context.setResource(resource);
        connection()->setContext(context);
        cookieData.resourceId = resourceId;
    }

    connection()->setState(Server::Authenticated);

    Protocol::LoginResponse response;
    response.setCookie(cookie);

    return successResponse(std::move(response));
}
