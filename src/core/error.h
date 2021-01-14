/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QString>

class QDebug;

namespace Akonadi
{

class AKONADICORE_EXPORT Error
{
public:
    explicit Error() = default;
    Error(int code, const QString &message)
        : mCode(code)
        , mMessage(message)
    {
    }

    Error(const Error &) = default;
    Error(Error &&) = default;

    Error &operator=(const Error &) = default;
    Error &operator=(Error &&) = default;

    explicit operator bool() const
    {
        return mCode != 0;
    }

    bool operator==(const Error &other) const
    {
        return std::tie(mCode, mMessage) == std::tie(other.mCode, other.mMessage);
    }

    bool operator!=(const Error &other) const
    {
        return !(*this == other);
    }

    int code() const
    {
        return mCode;
    }
    QString message() const
    {
        return mMessage;
    }

private:
    int mCode = 0;
    QString mMessage;
};

AKONADICORE_EXPORT QDebug operator<<(QDebug dbg, const Error &error);

} // namespace Akonadi
