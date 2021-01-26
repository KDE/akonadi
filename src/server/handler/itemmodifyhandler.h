/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_ITEMMODIFYHANDLER_H_
#define AKONADI_ITEMMODIFYHANDLER_H_

#include "entities.h"
#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler

  Handler for the item modification command.

  <h4>Semantics</h4>
  Modifies the selected items. Item selection can happen within the usual three scopes:
  - based on a uid set relative to the currently selected collection
  - based on a uid set (UID)
  - based on a list of remote identifiers within the currently selected collection (RID)

  The following item properties can be modified:
  - the remote identifier (@c REMOTEID)
  - the remote revision (@c REMOTEREVISION)
  - the global identifier (@c GID)
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
  - invalidating the cache
  - modifying the remote identifier
  - modifying the remote revision

  Conflict detection:
  - only available when modifying a single item
  - requires the previous item revision to be provided (@c REV)
*/

class ItemModifyHandler : public Handler
{
public:
    ItemModifyHandler(AkonadiServer &akonadi);
    ~ItemModifyHandler() override = default;

    bool parseStream() override;

private:
    bool replaceFlags(const PimItem::List &items, const QSet<QByteArray> &flags, bool &flagsChanged);
    bool addFlags(const PimItem::List &items, const QSet<QByteArray> &flags, bool &flagsChanged);
    bool deleteFlags(const PimItem::List &items, const QSet<QByteArray> &flags, bool &flagsChanged);
    bool replaceTags(const PimItem::List &items, const Scope &tags, bool &tagsChanged);
    bool addTags(const PimItem::List &items, const Scope &tags, bool &tagsChanged);
    bool deleteTags(const PimItem::List &items, const Scope &tags, bool &tagsChanged);
};

} // namespace Server
} // namespace Akonadi

#endif
