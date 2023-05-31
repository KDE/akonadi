/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the item move command.

  <h4>Semantics</h4>
  Moves the selected items. Item selection can happen within the usual three scopes:
  - based on a uid set relative to the currently selected collection
  - based on a global uid set (UID)
  - based on a list of remote identifiers within the currently selected collection (RID)

  Destination is a collection id.
*/
class ItemMoveHandler : public Handler
{
public:
    ItemMoveHandler(AkonadiServer &akonadi);
    ~ItemMoveHandler() override = default;

    bool parseStream() override;

private:
    void itemsRetrieved(const QList<qint64> &ids);

    Collection mDestination;
};

} // namespace Server
} // namespace Akonadi
