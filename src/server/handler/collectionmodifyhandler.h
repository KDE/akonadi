/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONMODIFYHANDLER_H_
#define AKONADI_COLLECTIONMODIFYHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

/**
  @ingroup akonadi_server_handler

  This command is used to modify collections. Its syntax is similar to the STORE
  command.
*/
class CollectionModifyHandler: public Handler
{
public:
    CollectionModifyHandler(AkonadiServer &akonadi);
    ~CollectionModifyHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
