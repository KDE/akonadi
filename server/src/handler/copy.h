/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COPY_H
#define AKONADI_COPY_H

#include <handler.h>
#include <entities.h>

namespace Akonadi {

/**
  @ingroup akonadi_server_handler

  Handler for the COPY command.

  This command is used to copy a set of items into the specific collection. It
  is syntactically identical to the IMAP COPY command.

  The copied items differ in the following points from the originals:
  - new unique id
  - empty remote id
  - possible located in a different collection (and thus resource)

  <h4>Syntax</h4>

  Request:
  @verbatim
  request = tag " COPY " seqeunce-set " " collection-id
  @endverbatim

  There is only the usual status response indicating success or failure of the
  COPY command
 */
class AKONADI_SERVER_EXPORT Copy : public Handler
{
  public:
    bool handleLine(const QByteArray& line);

  private:
    bool copyItem( const PimItem& item, const Location &target );
};

}

#endif
