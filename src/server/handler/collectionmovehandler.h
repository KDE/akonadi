/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONMOVEHANDLER_H_
#define AKONADI_COLLECTIONMOVEHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  Handler for the MoveCollection command

  This command is used to move a set of collections into another collection, including
  all sub-collections and their content.
*/
class CollectionMoveHandler : public Handler
{
public:
    CollectionMoveHandler(AkonadiServer &akonadi);
    ~CollectionMoveHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
