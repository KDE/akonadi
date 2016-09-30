/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_SERVERMANAGER_P_H
#define AKONADI_SERVERMANAGER_P_H

#include "akonadicore_export.h"

class QString;

namespace Akonadi
{

namespace Internal
{

AKONADICORE_EXPORT int serverProtocolVersion();
AKONADICORE_EXPORT void setServerProtocolVersion(int version);
AKONADICORE_EXPORT uint generation();
AKONADICORE_EXPORT void setGeneration(uint generation);

enum ClientType {
    User,
    Agent,
    Resource
};
AKONADICORE_EXPORT ClientType clientType();
AKONADICORE_EXPORT void setClientType(ClientType type);

}

}
#endif
