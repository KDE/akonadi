/*
 * SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

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
