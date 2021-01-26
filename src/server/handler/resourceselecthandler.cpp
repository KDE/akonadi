/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "resourceselecthandler.h"

#include "connection.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ResourceSelectHandler::ResourceSelectHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool ResourceSelectHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::SelectResourceCommand>(m_command);

    CommandContext context = connection()->context();
    if (cmd.resourceId().isEmpty()) {
        context.setResource({});
        connection()->setContext(context);
        return successResponse<Protocol::SelectResourceResponse>();
    }

    const Resource res = Resource::retrieveByName(cmd.resourceId());
    if (!res.isValid()) {
        return failureResponse(cmd.resourceId() % QStringLiteral(" is not a valid resource identifier"));
    }

    context.setResource(res);
    connection()->setContext(context);

    return successResponse<Protocol::SelectResourceResponse>();
}
