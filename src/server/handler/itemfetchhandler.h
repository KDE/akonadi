/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_ITEMFETCHHANDLER_H_ 
#define AKONADI_ITEMFETCHHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the fetch command.
*/
class ItemFetchHandler: public Handler
{
public:
    ItemFetchHandler(AkonadiServer &akonadi);
    ~ItemFetchHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
