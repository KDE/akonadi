/***************************************************************************
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "clientcapabilities.h"

using namespace Akonadi::Server;

ClientCapabilities::ClientCapabilities()
    : m_notificationMessageVersion(0)
    , m_noPayloadPath(false)
    , m_serverSideSearch(false)
    , m_akAppendStreaming(false)
    , m_directStreaming(false)
{
}

bool ClientCapabilities::isEmpty()
{
    return m_notificationMessageVersion == 0;
}

int ClientCapabilities::notificationMessageVersion() const
{
    return m_notificationMessageVersion;
}

void ClientCapabilities::setNotificationMessageVersion(int version)
{
    if (version <= 0) {
        return; // invalid
    }
    m_notificationMessageVersion = version;
}

bool ClientCapabilities::noPayloadPath() const
{
    return m_noPayloadPath;
}

void ClientCapabilities::setNoPayloadPath(bool noPayloadPath)
{
    m_noPayloadPath = noPayloadPath;
}

bool ClientCapabilities::serverSideSearch() const
{
    return m_serverSideSearch;
}

void ClientCapabilities::setServerSideSearch(bool serverSideSearch)
{
    m_serverSideSearch = serverSideSearch;
}

bool ClientCapabilities::akAppendStreaming() const
{
    return m_akAppendStreaming;
}

void ClientCapabilities::setAkAppendStreaming(bool akAppendStreaming)
{
    m_akAppendStreaming = akAppendStreaming;
}

bool ClientCapabilities::directStreaming() const
{
    return m_directStreaming;
}

void ClientCapabilities::setDirectStreaming(bool directStreaming)
{
    m_directStreaming = directStreaming;
}
