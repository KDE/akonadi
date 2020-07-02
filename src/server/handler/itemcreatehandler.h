/***************************************************************************
 *   SPDX-FileCopyrightText: 2007 Robert Zwerus <arzie@dds.nl>             *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_ITEMCREATEHANDLER_H_
#define AKONADI_ITEMCREATEHANDLER_H_

#include "handler.h"
#include "entities.h"

namespace Akonadi
{
namespace Server
{


/**
  @ingroup akonadi_server_handler

  Handler for the X-AKAPPEND command.

  This command is used to append an item with multiple parts.

 */
class ItemCreateHandler: public Handler
{
public:
    ItemCreateHandler(AkonadiServer &akonadi);
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
