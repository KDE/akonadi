/***************************************************************************
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
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

#ifndef AKONADI_ITEMCREATEHANDLER_H_
#define AKONADI_ITEMCREATEHANDLER_H_

#include "handler.h"
#include "entities.h"

namespace Akonadi
{
namespace Server
{

class Transaction;

/**
  @ingroup akonadi_server_handler

  Handler for the X-AKAPPEND command.

  This command is used to append an item with multiple parts.

 */
class ItemCreateHandler: public Handler
{
public:
    ~ItemCreateHandler() override = default;

    bool parseStream() override;

private:
    bool buildPimItem(const Protocol::CreateItemCommand &cmd,
                      PimItem &item,
                      Collection &parentCollection);

    bool insertItem(const Protocol::CreateItemCommand &cmd,
                    PimItem &item,
                    const Collection &parentCollection);

    bool mergeItem(const Protocol::CreateItemCommand &cmd,
                   PimItem &newItem,
                   PimItem &currentItem,
                   const Collection &parentCollection);

    bool sendResponse(const PimItem &item, Protocol::CreateItemCommand::MergeModes mergeModes);

    bool notify(const PimItem &item, bool seen, const Collection &collection);
    bool notify(const PimItem &item, const Collection &collection,
                const QSet<QByteArray> &changedParts);

    void recoverFromMultipleMergeCandidates(const PimItem::List &items, const Collection &collection);
};

} // namespace Server
} // namespace Akonadi

#endif
