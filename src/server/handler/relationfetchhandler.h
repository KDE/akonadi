/***************************************************************************
 *   SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>    *
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

  Handler for the RELATIONFETCH command.
 */
class RelationFetchHandler : public Handler
{
public:
    RelationFetchHandler(AkonadiServer &akonadi);
    ~RelationFetchHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi
