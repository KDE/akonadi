/*
    SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "tl_expected/expected.hpp"

#include "error.h"

namespace Akonadi
{

template<typename T, typename E = Akonadi::Error>
using Expected = tl::expected<T, E>;

template<typename T>
inline Expected<T> makeExpected(T &&t)
{
    return Expected<T>{std::forward<T>(t)};
}

inline Expected<void> makeExpected()
{
    return Expected<void>{};
}

template<typename E = Akonadi::Error>
using Unexpected = tl::unexpected<E>;

} // namespace Akonadi
