/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TAGCREATEHANDLER_H_
#define AKONADI_TAGCREATEHANDLER_H_

#include "handler.h"

namespace Akonadi
{
namespace Server
{

class TagCreateHandler: public Handler
{
public:
    TagCreateHandler(AkonadiServer &akonadi);
    ~TagCreateHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_TAGAPPEND_H
