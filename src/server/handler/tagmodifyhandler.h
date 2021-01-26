/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGMODIFYHANDLER_H_
#define AKONADI_TAGMODIFYHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{
class TagModifyHandler : public Handler
{
public:
    TagModifyHandler(AkonadiServer &akonadi);
    ~TagModifyHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_TAGMODIFYHANDLER_H_
