/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RESOURCESELECTHANDLER_H_
#define AKONADI_RESOURCESELECTHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the resource selection command.

  <h4>Semantics</h4>
  Limits the scope of remote id based operations. Remote ids of collections are only guaranteed
  to be unique per resource, so this command should be issued before running any RID based
  collection commands.
*/
class ResourceSelectHandler : public Handler
{
public:
    ResourceSelectHandler(AkonadiServer &akonadi);
    ~ResourceSelectHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
