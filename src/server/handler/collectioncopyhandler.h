/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_COLLECTIONCOPYHANDLER_H_
#define AKONADI_COLLECTIONCOPYHANDLER_H_

#include "handler/itemcopyhandler.h"
#include "entities.h"

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
    ~CollectionCopyHandler() override = default;

    bool parseStream() override;

private:
    bool copyCollection(const Collection &source, const Collection &target);
};

} // namespace Server
} // namespace Akonadi

#endif
