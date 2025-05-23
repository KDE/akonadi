/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiprivate_export.h"

#include <exception>
#include <iostream>

#include <QByteArray>

namespace Akonadi
{
class AKONADIPRIVATE_EXPORT ProtocolException : public std::exception
{
public:
    explicit ProtocolException(const char *what)
        : std::exception()
        , mWhat(what)
    {
        std::cerr << "ProtocolException thrown:" << what << std::endl;
    }

    ProtocolException(const ProtocolException &) = default;
    ProtocolException &operator=(const ProtocolException &) = default;

    const char *what() const throw() override
    {
        return mWhat.constData();
    }

private:
    QByteArray mWhat;
};
} // namespace Akonadi
