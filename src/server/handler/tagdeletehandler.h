/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "handler.h"

namespace Akonadi
{
namespace Server
{
class TagDeleteHandler : public Handler
{
public:
    TagDeleteHandler(AkonadiServer &akonadi);
    ~TagDeleteHandler() override = default;

    bool parseStream() override;
};

} // namespace Server
} // namespace Akonadi
