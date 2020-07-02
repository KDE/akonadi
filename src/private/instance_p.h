/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef AKONADI_INSTANCE_P_H
#define AKONADI_INSTANCE_P_H

#include "akonadiprivate_export.h"

class QString;

namespace Akonadi
{

namespace Instance
{
AKONADIPRIVATE_EXPORT bool hasIdentifier();
AKONADIPRIVATE_EXPORT void setIdentifier(const QString &identifier);
AKONADIPRIVATE_EXPORT QString identifier();
}
}

#endif // AKONADI_INSTANCE_P_H
