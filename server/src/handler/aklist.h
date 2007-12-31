/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AKLIST_H
#define AKONADI_AKLIST_H

#include <entities.h>
#include <handler.h>
#include "akonadi_export.h"

namespace Akonadi {

/**
  @ingroup akonadi_server_handler

  Handler for the X-AKLIST command.

  This command is used to get a (limited) listing of the available collections.
  It is different from the LIST command and is more similar to FETCH.

  <h4>Syntax</h4>

  Request:
  @verbatim
  request = tag " " command " " collection-id " " depth " (" filter-list ")"
  command = "X-AKLIST" | "X-AKLSUB"
  depth = number | "INF"
  filter-list = *(filter-key " " filter-value)
  filter-key = "RESOURCE"
  @endverbatim

  @c X-AKLIST will include all known collections, @c X-AKLSUB only those that are
  subscribed or contains subscribed collections (cf. RFC 3591, LIST vs. LSUB).

  @c depths chooses between recursive (@c INF), flat (1) and local (0, ie. just the
  base collection) listing, 0 indicates the root collection.
  The @c filter-list is used to restrict the listing to collection of a specific
  resource.

  Response:
  @verbatim
  response = "*" collection-id " " parent-id " ("attribute-list")"
  attribute-list = *(attribute-identifier " " attribute-value)
  attribute-identifier = "NAME" | "MIMETYPE" | "REMOTEID" | "RESOURCE" | custom-attr-identifier
  @endverbatim

  The name is encoded as an quoted UTF-8 string. There is no order defined for the
  single responses.
 */
class AKONADI_SERVER_EXPORT AkList : public Handler
{
  public:
    AkList();
    ~AkList();

    virtual bool handleLine(const QByteArray& line);

  private:
    bool listCollection( const Location &root, int depth );

  private:
    Resource mResource;
    bool mOnlySubscribed;

};

}

#endif
