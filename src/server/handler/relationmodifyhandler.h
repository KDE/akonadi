/*
    SPDX-FileCopyrightText: 2014 Christian Mollekop <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_RELATIONMODIFYHANDLER_H_
#define AKONADI_RELATIONMODIFYHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

class Relation;

class RelationModifyHandler: public Handler
{
public:
    RelationModifyHandler(AkonadiServer &akonadi);
    ~RelationModifyHandler() override = default;

    bool parseStream() override;

private:
    Relation fetchRelation(qint64 leftId, qint64 rightId, qint64 typeId);
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_RELATIONMODIFYHANDLER_H_
