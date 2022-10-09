/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
 * @ingroup akonadi_server_handler
 *
 * Handler for the LINK and UNLINK commands.
 *
 * These commands are used to add and remove references of a set of items to a
 * virtual collection.
 */
class ItemLinkHandler : public Handler
{
public:
    ItemLinkHandler(AkonadiServer &akonadi);
    ~ItemLinkHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi
