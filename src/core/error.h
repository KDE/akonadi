/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#if ((defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || __cplusplus < 201703L)
#error "Usage of the Akonadi Async Storage API requires C++17 or higher"
#endif

#include "akonadicore_export.h"
#include "error.h"

#include <QString>

class QDebug;

namespace Akonadi
{

class AKONADICORE_EXPORT Error
{
public:
    explicit Error() = default;
    Error(int code, const QString &message)
        : mCode(code), mMessage(message)
    {}

    Error(const Error &) = default;
    Error(Error &&) = default;

    Error &operator=(const Error &) = default;
    Error &operator=(Error &&) = default;

    explicit operator bool() const { return mCode != 0; }

    int code() const { return mCode; }
    QString message() const { return mMessage; }

private:
    int mCode = 0;
    QString mMessage;
};

AKONADICORE_EXPORT QDebug operator<<(QDebug dbg, const Error &error);

} // namespace Akonadi
