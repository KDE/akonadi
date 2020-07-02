/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#ifndef AKONADI_LOGOUTHANDLER_H_
#define AKONADI_LOGOUTHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the logout command.
 */
class LogoutHandler: public Handler
{
public:
    LogoutHandler(AkonadiServer &akonadi);
    ~LogoutHandler() override = default;

    bool parseStream() override;

};

} // namespace Server
} // namespace Akonadi

#endif
