/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "logouthandler.h"

using namespace Akonadi;
using namespace Akonadi::Server;

LogoutHandler::LogoutHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool LogoutHandler::parseStream()
{
    sendResponse<Protocol::LogoutResponse>();

    connection()->setState(LoggingOut);
    return true;
}
