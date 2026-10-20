/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "protocol_exception_p.h"

#include "akonadiprivate_debug.h"

using namespace Akonadi;

ProtocolException::ProtocolException(const char *what)
    : std::exception()
    , mWhat(what)
{
    qCWarning(AKONADIPRIVATE_LOG) << "ProtocolException thrown:" << what;
}
