/*
    SPDX-FileCopyrightText: 2018-2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

namespace Akonadi
{
static const auto IsNull = [](auto ptr) {
    return !(bool)ptr;
};
static const auto IsNotNull = [](auto ptr) {
    return (bool)ptr;
};

} // namespace Akonadi

