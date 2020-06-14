/*
    SPDX-FileCopyrightText: 2014 Christian Mollekopf <mollekopf@kolabsys.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "error.h"

#include <QDebug>

using namespace Akonadi;

QDebug Akonadi::operator<<(QDebug dbg, const Error &error)
{
    dbg.noquote().nospace() << error.message() << " (" << error.code() << ")";
    return dbg;
}

