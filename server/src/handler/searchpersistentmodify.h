/***************************************************************************
 *   Copyright (C) 2010 by Till Adam <adam@kde.org>                   *
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

#ifndef AKONADISEARCHPERSISTENTMODIFY_H
#define AKONADISEARCHPERSISTENTMODIFY_H

#include <handler.h>

namespace Akonadi {

/**
  @ingroup akonadi_server_handler

  Handler for the search_modify command.

  A persistent search modification can have the following forms:
  @verbatim
  123 SEARCH_MODIFY <collection_id> <query>
  @endverbatim
*/
class SearchPersistentModify : public Handler
{
  Q_OBJECT
  public:
    SearchPersistentModify();

    ~SearchPersistentModify();

    bool parseStream();
};

}

#endif
