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

#ifndef AKONADICONTROL_AGENTINSTANCE_H
#define AKONADICONTROL_AGENTINSTANCE_H

#include "controlinterface.h"
#include "statusinterface.h"
#include "resourceinterface.h"
#include "tracerinterface.h"

#include <QDBusError>
#include <QString>
#include <QStringList>

#include <boost/shared_ptr.hpp>

namespace Akonadi {
  class ProcessControl;
}

class AgentManager;
class AgentType;

/**
 * Represents one agent instance and takes care of communication with it.
 */
class AgentInstance : public QObject
{
  Q_OBJECT
  public:
    typedef boost::shared_ptr<AgentInstance> Ptr;

    explicit AgentInstance( AgentManager *manager );

    QString identifier() const { return mIdentifier; }
    void setIdentifier( const QString &identifier ) { mIdentifier = identifier; }

    QString agentType() const { return mType; }
    int status() const { return mStatus; }
    QString statusMessage() const { return mStatusMessage; }
    int progress() const { return mPercent; }
    bool isOnline() const { return mOnline; }
    QString resourceName() const { return mResourceName; }

    bool start( const AgentType &agentInfo );
    void quit();
    void cleanup();
    void restartWhenIdle();

    bool hasResourceInterface() const { return mResourceInterface; }
    bool hasAgentInterface() const { return mAgentControlInterface && mAgentStatusInterface; }

    org::freedesktop::Akonadi::Agent::Control* controlInterface() const { return mAgentControlInterface; }
    org::freedesktop::Akonadi::Agent::Status* statusInterface() const { return mAgentStatusInterface; }
    org::freedesktop::Akonadi::Resource* resourceInterface() const { return mResourceInterface; }

    bool obtainAgentInterface();
    bool obtainResourceInterface();

  private slots:
    void statusChanged( int status, const QString &statusMsg );
    void statusStateChanged( int status );
    void statusMessageChanged( const QString &msg );
    void percentChanged( int percent );
    void warning( const QString &msg );
    void error( const QString &msg );
    void onlineChanged( bool state );
    void resourceNameChanged( const QString &name );

    void refreshAgentStatus();
    void refreshResourceStatus();

    void errorHandler( const QDBusError &error );

  private:
    QString mIdentifier;
    QString mType;
    AgentManager *mManager;
    Akonadi::ProcessControl *mController;
    org::freedesktop::Akonadi::Agent::Control *mAgentControlInterface;
    org::freedesktop::Akonadi::Agent::Status *mAgentStatusInterface;
    org::freedesktop::Akonadi::Resource *mResourceInterface;

    int mStatus;
    QString mStatusMessage;
    int mPercent;
    QString mResourceName;
    bool mOnline;

};

#endif
