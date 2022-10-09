/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
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

  Handler for the search_store search_delete commands.
*/
class SearchCreateHandler : public Handler
{
public:
    SearchCreateHandler(AkonadiServer &akonadi);
    ~SearchCreateHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi
