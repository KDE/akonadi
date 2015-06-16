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

#ifndef AKONADI_MOVEJOBIMPL_P_H
#define AKONADI_MOVEJOBIMPL_P_H

#include "collection.h"
#include "job.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <akonadi/private/protocol_p.h>

#include <KLocalizedString>

namespace Akonadi {

/** Shared implementation details between item and collection move jobs. */
template <typename T, typename MoveJob> class MoveJobImpl : public JobPrivate
{
public:
    MoveJobImpl(Job *parent)
        : JobPrivate(parent)
    {
    }

    inline void sendCommand(const char *asapCommand)
    {
        MoveJob *q = static_cast<MoveJob *>(q_func());  // Job would be enough already, but then we don't have access to the non-public stuff...
        if (objectsToMove.isEmpty()) {
            q->setError(Job::Unknown);
            q->setErrorText(i18n("No objects specified for moving"));
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
            command += ProtocolHelper::entitySetToByteArray(objectsToMove, asapCommand);
        } catch (const std::exception &e) {
            q->setError(Job::Unknown);
            q->setErrorText(QString::fromUtf8(e.what()));
            q->emitResult();
            return;
        }
        command += ' ';
        // with all the checks done before this indicates now whether this is a UID or RID based operation
        if (objectsToMove.first().isValid()) {
            command += QByteArray::number(destination.id());
        } else {
            command += ImapParser::quote(destination.remoteId().toUtf8());
        }

        // Source is optional
        if (source.isValid()) {
            command += ' ' + QByteArray::number(source.id());
        } else if (!source.remoteId().isEmpty()) {
            command += ' ' + ImapParser::quote(source.remoteId().toUtf8());
        }
        command += '\n';
        writeData(command);
    }

    typename T::List objectsToMove;
    Collection destination;
    Collection source;
};

}

#endif
