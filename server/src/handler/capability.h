/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AKONADICAPABILITY_H
#define AKONADICAPABILITY_H

#include <handler.h>

namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the capability command.

  <h4>Syntax</h4>
  Request:
  @verbatim
  <tag> CAPABILITY <client capability list>
  @endverbatim

  Response:
  @verbatim
  [<tag> <server capability list>]
  <tag> OK
  @endverbatim

  <h4>Client Capabilities</h4>
  - @c NOTIFY version - version of the notification message format
  - @c NOPAYLOADPATH - only filename of external payload file is expected

  <h4>Server Capabilities</h4>
  None defined yet.

  @since ASAP 32, Akonadi 1.10
 */
class Capability : public Handler
{
    Q_OBJECT
public:
    Capability();
    ~Capability();

    bool parseStream();
};

} // namespace Server
} // namespace Akonadi

#endif
