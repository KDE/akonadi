/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_DELETE_H
#define AKONADI_DELETE_H

#include <handler.h>
#include <entities.h>
#include <scope.h>

namespace Akonadi {
namespace Server {

/**
  @ingroup akonadi_server_handler

  Handler for the collection deletion command.

  This commands deletes the selected collections including all their content
  and that of any child collection.

  <h4>Syntax</h4>

  Request:
  @verbatim
  request = tag [" RID"] " DELETE " collection-ids
  @endverbatim

  @c collection-ids is the set of collections that should be deleted, either as UID-set
  or as a list of RIDs (in case the @c RID prefix is given).

  There is only the usual status response indicating success or failure of the
  DELETE command
*/
class Delete : public Handler
{
    Q_OBJECT
public:
    Delete(Scope scope);
    bool parseStream();

private:
    bool deleteRecursive(Collection &col);
    Scope m_scope;

};

} // namespace Server
} // namespace Akonadi

#endif
