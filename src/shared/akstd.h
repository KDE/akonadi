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

#ifndef AKSTD_H_
#define AKSTD_H_

#include <functional>

#include <QString>
#include <QHash>

/// A glue between Qt and the standard library

namespace std {

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
template<>
struct hash<QString> {
    using argument_type = QString;
    using result_type = std::size_t;

    result_type operator()(const QString &s) const noexcept
    {
        return qHash(s);
    }
};
#endif

}

#endif
