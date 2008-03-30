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

#ifndef AKONADI_AGENTINFO_H
#define AKONADI_AGENTINFO_H

#include "controlinterface.h"
#include "statusinterface.h"
#include "resourceinterface.h"
#include "tracerinterface.h"

#include <QString>
#include <QStringList>

namespace Akonadi {
  class ProcessControl;
}

class AgentManager;
class QSettings;

class AgentInfo
{
  public:
    AgentInfo();
    bool load( const QString &fileName, AgentManager *manager );
    void save( QSettings *config ) const;

    QString identifier;
    QString name;
    QString comment;
    QString icon;
    QStringList mimeTypes;
    QStringList capabilities;
    QString exec;
    uint instanceCounter;

    static QLatin1String CapabilityUnique;
    static QLatin1String CapabilityResource;
    static QLatin1String CapabilityAutostart;
};

class AgentInstanceInfo
{
  public:
    AgentInstanceInfo();
    bool start( const AgentInfo &agentInfo, AgentManager* manager );
    bool isResource() const { return resourceInterface; }

    QString identifier;
    QString agentType;
    Akonadi::ProcessControl *controller;
    org::kde::Akonadi::Agent::Control *agentControlInterface;
    org::kde::Akonadi::Agent::Status *agentStatusInterface;
    org::kde::Akonadi::Resource *resourceInterface;
};


#endif
