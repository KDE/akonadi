/*
 * Copyright 2015  Daniel Vratil <dvratil@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
