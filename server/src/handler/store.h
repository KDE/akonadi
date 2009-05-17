/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2009 by Volker Krause <vkrause@kde.org>                 *
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

#ifndef AKONADISTORE_H
#define AKONADISTORE_H

#include <handler.h>
#include <scope.h>

#include "storage/entity.h"
#include "libs/imapset_p.h"

namespace Akonadi {

class PimItem;

/**
  @ingroup akonadi_server_handler

  Handler for the item modification command.

  <h4>Syntax</h4>
  One of the following three:
  @verbatim
  <tag> STORE <uid-set> <modifications>
  <tag> UID MOVE <uid-set> [<revision-check>] <modifications>
  <tag> RID MOVE <remote-identifiers> [<revision-check>] <modifications>
  @endverbatim

  @c revision-check is one of the following and allowed iff one item was selected for modification:
  @verbatim
  NOREV
  REV <int>
  @endverbatim

  @c modifcations is a parenthesized list containing any of the following:
  @verbatim
  SIZE <int>
  [+-]FLAGS <flag-list>
  REMOTEID <remote-identifier>
  DIRTY
  <attribute-id> <attribute-value>
  <part-id> <part-value>
  @endverbatim

  <h4>Semantics</h4>
  Modifies the selected items. Item selection can happen within the usual three scopes:
  - based on a uid set relative to the currently selected collection
  - based on a global uid set (UID)
  - based on a list of remote identifiers within the currently selected collection (RID)

  The following item properties can be mofidied:
  - the remote identifier (@c REMOTEID)
  - resetting the dirty flag indication local changes not yet replicated to the backend (@c DIRTY)
  - adding/deleting/setting item flags (@c FLAGS)
  - setting the item size hint (@c SIZE)
  - changing item attributes
  - changing item payload parts

  If multiple items are selected only the following operations are valid:
  - adding flags
  - removing flags
  - settings flags

  The following operations are only allowed by resources:
  - resetting the dirty flag
  - modifying the remote identifier

  Conflict detection:
  - only available when modifying a single item
  - requires the previous item revision to be provided (@c REV)
*/

class Store : public Handler
{
  Q_OBJECT

  public:
    Store( Scope::SelectionScope scope );
    bool parseStream();

  private:
    enum Operation
    {
      Replace,
      Add,
      Delete
    };

    void parseCommand();

    bool replaceFlags( const PimItem &item, const QList<QByteArray> &flags );
    bool addFlags( const PimItem &item, const QList<QByteArray> &flags );
    bool deleteFlags( const PimItem &item, const QList<QByteArray> &flags );
    void sendPimItemResponse( const PimItem &pimItem );

  private:
    Scope mScope;
    int mPos;
    qint64 mPreviousRevision;
    qint64 mSize;
};

}

#endif
