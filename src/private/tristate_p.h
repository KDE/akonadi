/*
 *    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef AKONADI_PRIVATE_TRISTATE_P_H_
#define AKONADI_PRIVATE_TRISTATE_P_H_

#include <QDebug>
#include <QMetaType>

#include "akonadiprivate_export.h"

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

#endif
