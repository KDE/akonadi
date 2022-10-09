/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "handler.h"

namespace Akonadi
{
namespace Server
{
class Collection;

/**
  @ingroup akonadi_server_handler

  Handler for the collection deletion command.

  This commands deletes the selected collections including all their content
  and that of any child collection.
*/
class CollectionDeleteHandler : public Handler
{
public:
    CollectionDeleteHandler(AkonadiServer &akonadi);
    ~CollectionDeleteHandler() override = default;

    bool parseStream() override;

private:
    bool deleteRecursive(Collection &col);
};

} // namespace Server
} // namespace Akonadi
