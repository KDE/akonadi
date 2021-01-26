/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Ingo Kloecker <kloecker@kde.org>         *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#ifndef AKONADI_COLLECTIONCREATEHANDLER_H_
#define AKONADI_COLLECTIONCREATEHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{
/**
  @ingroup akonadi_server_handler
 */
class CollectionCreateHandler : public Handler
{
public:
    CollectionCreateHandler(AkonadiServer &akonadi);
    ~CollectionCreateHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif
