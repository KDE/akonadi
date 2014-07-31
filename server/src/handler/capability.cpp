/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
#include "response.h"
#include "imapstreamparser.h"
#include "clientcapabilities.h"
#include "connection.h"

#include <libs/protocol_p.h>

#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Server;

Capability::Capability()
    : Handler()
{
}

Capability::~Capability()
{
}

bool Capability::parseStream()
{
    ClientCapabilities capabilities;

    m_streamParser->beginList();
    while (!m_streamParser->atListEnd()) {
        const QByteArray capability = m_streamParser->readString();
        if (capability.isEmpty()) {
            break; // shouldn't happen
        }
        if (capability == AKONADI_PARAM_CAPABILITY_NOTIFY) {
            capabilities.setNotificationMessageVersion(m_streamParser->readNumber());
        } else if (capability == AKONADI_PARAM_CAPABILITY_NOPAYLOADPATH) {
            capabilities.setNoPayloadPath(true);
        } else if (capability == AKONADI_PARAM_CAPABILITY_SERVERSEARCH) {
            capabilities.setServerSideSearch(true);
        } else if (capability == AKONADI_PARAM_CAPABILITY_AKAPPENDSTREAMING) {
            capabilities.setAkAppendStreaming(true);
        } else if (capability == AKONADI_PARAM_CAPABILITY_DIRECTSTREAMING) {
            capabilities.setDirectStreaming(true);
        } else {
            qDebug() << Q_FUNC_INFO << "Unknown client capability:" << capability;
        }
    }

    connection()->setCapabilities(capabilities);

    Response response;
    response.setSuccess();
    response.setTag(tag());
    response.setString("CAPABILITY completed");
    Q_EMIT responseAvailable(response);
    return true;
}
