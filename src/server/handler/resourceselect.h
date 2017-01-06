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

#ifndef AKONADI_RESOURCESELECT_H
#define AKONADI_RESOURCESELECT_H

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
class  ResourceSelect : public Handler
{
    Q_OBJECT
public:
    bool parseStream() Q_DECL_OVERRIDE;
};

} // namespace Server
} // namespace Akonadi

#endif
