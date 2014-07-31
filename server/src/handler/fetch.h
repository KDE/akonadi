/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#ifndef AKONADIFETCH_H
#define AKONADIFETCH_H

#include <handler.h>

#include "scope.h"

namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the fetch command.

  Request syntax:
  @verbatim
  fetch-request = tag " " [scope-selector " "] "FETCH " scope " " fetch-parameters " " part-list
  scope-selector = ["UID" / "RID"]
  fetch-parameters = ["FULLPAYLOAD" / "CACHEONLY" / "CACHEONLY" / "EXTERNALPAYLOAD" / "ANCESTORS " depth]
  part-list = "(" *(part-id) ")"
  depth = "0" / "1" / "INF"
  @endverbatim

  Semantics:
  - @c FULLPAYLOAD: Retrieve the full payload
  - @c CACHEONLY: Restrict retrieval to parts already in the cache, even if more parts have been requested.
  - @c EXTERNALPAYLOAD: Indicate the capability to retrieve parts via the filesystem instead over the socket
  - @c ANCESTORS: Indicate the desired ancestor collection depth (0 is the default)
 */
class Fetch : public Handler
{
    Q_OBJECT
public:
    Fetch(Scope::SelectionScope scope);

    bool parseStream();

private:
    Scope mScope;
};

} // namespace Server
} // namespace Akonadi

#endif
