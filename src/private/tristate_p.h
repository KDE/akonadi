/*
 *    Copyright (c) 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 *    This library is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU Library General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or (at your
 *    option) any later version.
 *
 *    This library is distributed in the hope that it will be useful, but WITHOUT
 *    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *    License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to the
 *    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *    02110-1301, USA.
 */

#ifndef AKONADI_PRIVATE_TRISTATE_P_H_
#define AKONADI_PRIVATE_TRISTATE_P_H_

#include <QtCore/QMetaType>
#include <QtCore/QDebug>

#include "akonadiprivate_export.h"

namespace Akonadi
{

enum class Tristate : qint8
{
    False     = 0,
    True      = 1,
    Undefined = 2
};

}

Q_DECLARE_METATYPE(Akonadi::Tristate)

AKONADIPRIVATE_EXPORT QDebug operator<<(QDebug dbg, Akonadi::Tristate tristate);

#endif