/*
 *    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "tristate_p.h"

using namespace Akonadi;

QDebug operator<<(QDebug dbg, Tristate tristate)
{
    switch (tristate) {
    case Tristate::True:
        return dbg << "True";
    case Tristate::False:
        return dbg << "False";
    case Tristate::Undefined:
        return dbg << "Undefined";
    }

    Q_ASSERT(false);
    return dbg;
}
