/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTBASE_P_H
#define AKONADI_AGENTBASE_P_H

#include "agentbase.h"
#include "tracerinterface.h"

#include <klocale.h>

namespace Akonadi {

class AgentBasePrivate
{
  public:
    AgentBasePrivate( AgentBase *parent );
    ~AgentBasePrivate();
    void init();

    QString defaultErrorMessage() const
    {
      return i18nc( "@info:status", "Error!" );
    }

    AgentBase *q_ptr;
    Q_DECLARE_PUBLIC( AgentBase )

    QString mId;

    QSettings *mSettings;

    Session *session;
    ChangeRecorder *monitor;

    org::kde::Akonadi::Tracer *mTracer;
};

}

#endif
