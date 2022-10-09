/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "entities.h"
#include "handler/itemcopyhandler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the CollectionCopyHandler command.

  This command is used to copy a single collection into another collection, including
  all sub-collections and their content.

  The copied items differ in the following points from the originals:
  - new unique id
  - empty remote id
  - possible located in a different collection (and thus resource)

  The copied collections differ in the following points from the originals:
  - new unique id
  - empty remote id
  - owning resource is the same as the one of the target collection
 */
class CollectionCopyHandler : public ItemCopyHandler
{
public:
    CollectionCopyHandler(AkonadiServer &akonadi);
    ~CollectionCopyHandler() override = default;

    bool parseStream() override;

private:
    bool copyCollection(const Collection &source, const Collection &target);
};

} // namespace Server
} // namespace Akonadi
