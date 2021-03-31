/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <utility>

#include <QString>
#include <QStringView>

/// Helper integration between Akonadi and Qt

namespace Akonadi
{
template<typename DPtr, typename Slot> auto akPrivSlot(DPtr &&dptr, Slot &&slot)
{
    return [&dptr, &slot](auto &&...args) {
        (dptr->*slot)(std::forward<decltype(args)>(args)...);
    };
}

} // namespace

inline QString operator""_qs(const char16_t *str, std::size_t len)
{
    return QString(reinterpret_cast<const QChar *>(str), len);
}

constexpr QStringView operator""_qsv(const char16_t *str, std::size_t len)
{
    return QStringView(str, len);
}

