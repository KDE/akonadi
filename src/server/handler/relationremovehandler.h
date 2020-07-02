/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RELATIONREMOVEHANDLER_H_
#define AKONADI_RELATIONREMOVEHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

class RelationRemoveHandler: public Handler
{
public:
    RelationRemoveHandler(AkonadiServer &akonadi);
    ~RelationRemoveHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_RELATIONREMOVEHANDLER_H_
