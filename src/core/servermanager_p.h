/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

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
    Resource,
};
AKONADICORE_EXPORT ClientType clientType();
AKONADICORE_EXPORT void setClientType(ClientType type);

}

}
