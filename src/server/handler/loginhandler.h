/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#pragma once

#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the login command.
*/
class LoginHandler : public Handler
{
public:
    LoginHandler(AkonadiServer &akonadi);
    ~LoginHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi
