/*
    Copyright (C) 2019  Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKQT_H_
#define AKQT_H_

#include <utility>

#include <QStringView>
#include <QString>

/// Helper integration between Akonadi and Qt

namespace Akonadi
{

template<typename DPtr, typename Slot>
auto akPrivSlot(DPtr &&dptr, Slot &&slot)
{
    return [&dptr, &slot](auto && ...args) {
        (dptr->*slot)(std::forward<decltype(args)>(args) ...);
    };
}

} // namespace

inline QString operator ""_qs(const char16_t *str, std::size_t len)
{
    return QString(reinterpret_cast<const QChar*>(str), len);
}

constexpr QStringView operator ""_qsv(const char16_t *str, std::size_t len)
{
    return QStringView(str, len);
}

#endif
