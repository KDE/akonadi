/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMDELETEHANDLER_H_
#define AKONADI_ITEMDELETEHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the item deletion command.

  <h4>Semantics</h4>
  Removes the selected items. Item selection can happen within the usual three scopes:
  - based on a uid set relative to the currently selected collection
  - based on a global uid set (UID)
  - based on a remote identifier within the currently selected collection (RID)
*/
class ItemDeleteHandler: public Handler
{
public:
    ItemDeleteHandler(AkonadiServer &akonadi);
    ~ItemDeleteHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
