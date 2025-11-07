/***************************************************************************
 *   SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>         *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_ITEMSYNCHANDLER_H_
#define AKONADI_ITEMSYNCHANDLER_H_

#include "handler.h"

namespace Akonadi::Server
{

class ItemSyncer;
class IncrementalItemSyncer;
class FullItemSyncer;
class ItemSyncHandler : public Handler
{
public:
    explicit ItemSyncHandler(AkonadiServer &akonadi);

    bool parseStream() override;

    std::unique_ptr<ItemSyncer> createSyncer(ItemSyncHandler *handler, bool incremental) const;

protected:
    quint64 nextTag();

    friend class ItemSyncer;
    friend class IncrementalItemSyncer;
    friend class FullItemSyncer;
};

} // namespace Akonadi::Server

#endif
