/*
    Copyright (c) 2008,2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_LINKJOBIMPL_P_H
#define AKONADI_LINKJOBIMPL_P_H

#include "collection.h"
#include "item.h"
#include "job.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <qdebug.h>
#include <KLocalizedString>

namespace Akonadi {

/** Shared implementation details between item and collection move jobs. */
template <typename LinkJob> class LinkJobImpl : public JobPrivate
{
public:
    LinkJobImpl(Job *parent)
        : JobPrivate(parent)
    {
    }

    inline void sendCommand(const char *asapCommand)
    {
        LinkJob *q = static_cast<LinkJob *>(q_func());  // Job would be enough already, but then we don't have access to the non-public stuff...
        if (objectsToLink.isEmpty()) {
            q->emitResult();
            return;
        }
        if (!destination.isValid() && destination.remoteId().isEmpty()) {
            q->setError(Job::Unknown);
            q->setErrorText(i18n("No valid destination specified"));
            q->emitResult();
            return;
        }

        QByteArray command = newTag();
        try {
            command += ProtocolHelper::entitySetToByteArray(Collection::List() << destination, asapCommand);
        } catch (const std::exception &e) {
            q->setError(Job::Unknown);
            q->setErrorText(QString::fromUtf8(e.what()));
            q->emitResult();
            return;
        }

        try {
            command += ProtocolHelper::entitySetToByteArray(objectsToLink, QByteArray());
        } catch (const std::exception &e) {
            q->setError(Job::Unknown);
            q->setErrorText(QString::fromUtf8(e.what()));
            q->emitResult();
            return;
        }
        command += '\n';

        writeData(command);
    }

    Item::List objectsToLink;
    Collection destination;
};

}

#endif
