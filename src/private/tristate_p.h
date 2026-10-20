/*
 *    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QMetaType>

#include "akonadiprivate_export.h"

class QDebug;

namespace Akonadi
{
enum class Tristate : qint8 {
    False = 0,
    True = 1,
    Undefined = 2,
};
}

Q_DECLARE_METATYPE(Akonadi::Tristate)

AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, Akonadi::Tristate tristate);
