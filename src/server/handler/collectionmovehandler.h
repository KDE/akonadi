/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
