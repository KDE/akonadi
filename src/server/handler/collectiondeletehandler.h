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

#ifndef AKONADI_COLLECTIONDELETEHANDLER_H_
#define AKONADI_COLLECTIONDELETEHANDLER_H_

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
class CollectionDeleteHandler: public Handler
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

#endif
