/***************************************************************************
 *   SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>    *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_RELATIONFETCHHANDLER_H_
#define AKONADI_RELATIONFETCHHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the RELATIONFETCH command.
 */
class RelationFetchHandler: public Handler
{
public:
    RelationFetchHandler(AkonadiServer &akonadi);
    ~RelationFetchHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
