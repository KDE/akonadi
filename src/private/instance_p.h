/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_INSTANCE_H
#define AKONADI_INSTANCE_H

#include "akonadiprivate_export.h"

class QString;

namespace Akonadi {

class AKONADIPRIVATE_EXPORT Instance
{
public:
    static bool hasIdentifier();
    static void setIdentifier(const QString &identifier);
    static QString identifier();

private:
    static void loadIdentifier();

    static QString sIdentifier;
};
}

#endif // AKONADI_INSTANCE_H
