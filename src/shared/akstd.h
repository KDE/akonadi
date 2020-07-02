/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
