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

#include "capability.h"

#ifndef AKONADI_CLIENTCAPABILITIES_H
#define AKONADI_CLIENTCAPABILITIES_H

namespace Akonadi {
namespace Server {

/** Describes the capabilities of a specific session.
    Filled by the CAPABILITY command.
 */
class ClientCapabilities
{
public:
    ClientCapabilities();

    /** Returns @c true if no capabilities have been set by the client.
     *  This is useful for detecting legacy clients.
     */
    bool isEmpty();

    int notificationMessageVersion() const;
    void setNotificationMessageVersion(int version);

    bool noPayloadPath() const;
    void setNoPayloadPath(bool noPayloadPath);

    void setServerSideSearch(bool serverSideSearch);
    bool serverSideSearch() const;

    bool akAppendStreaming() const;
    void setAkAppendStreaming(bool akAppendStreaming);

    bool directStreaming() const;
    void setDirectStreaming(bool directStreaming);

private:
    int m_notificationMessageVersion;
    int m_noPayloadPath : 1;
    int m_serverSideSearch : 1;
    int m_akAppendStreaming : 1;
    int m_directStreaming : 1;
};

} // namespace Server
} // namespace Akonadi

#endif // CLIENTCAPABILITIES_H
