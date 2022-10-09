/***************************************************************************
 *   SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>       *
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

  Handler for the FETCHTAG command.
 */
class TagFetchHandler : public Handler
{
public:
    TagFetchHandler(AkonadiServer &akonadi);
    ~TagFetchHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi
