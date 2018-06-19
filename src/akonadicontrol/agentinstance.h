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
#include "preprocessorinterface.h"
#include "searchinterface.h"

#include <private/dbus_p.h>

#include <QDBusError>
#include <QSharedPointer>
#include <QString>

class AgentManager;
class AgentType;

/**
 * Represents one agent instance and takes care of communication with it.
 *
 * The agent exposes multiple D-Bus interfaces. The Control and the Status
 * interfaces are implemented by all the agents. The Resource and Preprocessor
 * interfaces are obviously implemented only by the agents impersonating resources or
 * preprocessors.
 */
class AgentInstance : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<AgentInstance> Ptr;

    explicit AgentInstance(AgentManager *manager);
    virtual ~AgentInstance()
    {
    }

    /** Set/get the unique identifier of this AgentInstance */
    Q_REQUIRED_RESULT QString identifier() const
    {
        return mIdentifier;
    }

    void setIdentifier(const QString &identifier)
    {
        mIdentifier = identifier;
    }

    Q_REQUIRED_RESULT QString agentType() const
    {
        return mType;
    }

    Q_REQUIRED_RESULT int status() const
    {
        return mStatus;
    }

    Q_REQUIRED_RESULT QString statusMessage() const
    {
        return mStatusMessage;
    }

    Q_REQUIRED_RESULT int progress() const
    {
        return mPercent;
    }

    Q_REQUIRED_RESULT bool isOnline() const
    {
        return mOnline;
    }

    Q_REQUIRED_RESULT QString resourceName() const
    {
        return mResourceName;
    }

    virtual bool start(const AgentType &agentInfo) = 0;
    virtual void quit();
    virtual void cleanup();
    virtual void restartWhenIdle() = 0;
    virtual void configure(qlonglong windowId) = 0;

    Q_REQUIRED_RESULT bool hasResourceInterface() const
    {
        return mResourceInterface;
    }

    Q_REQUIRED_RESULT bool hasAgentInterface() const
    {
        return mAgentControlInterface && mAgentStatusInterface;
    }

    Q_REQUIRED_RESULT bool hasPreprocessorInterface() const
    {
        return mPreprocessorInterface;
    }

    org::freedesktop::Akonadi::Agent::Control *controlInterface() const
    {
        return mAgentControlInterface;
    }

    org::freedesktop::Akonadi::Agent::Status *statusInterface() const
    {
        return mAgentStatusInterface;
    }

    org::freedesktop::Akonadi::Agent::Search *searchInterface() const
    {
        return mSearchInterface;
    }

    org::freedesktop::Akonadi::Resource *resourceInterface() const
    {
        return mResourceInterface;
    }

    org::freedesktop::Akonadi::Preprocessor *preProcessorInterface() const
    {
        return mPreprocessorInterface;
    }

    Q_REQUIRED_RESULT bool obtainAgentInterface();
    Q_REQUIRED_RESULT bool obtainResourceInterface();
    Q_REQUIRED_RESULT bool obtainPreprocessorInterface();

protected Q_SLOTS:
    void statusChanged(int status, const QString &statusMsg);
    void advancedStatusChanged(const QVariantMap &status);
    void statusStateChanged(int status);
    void statusMessageChanged(const QString &msg);
    void percentChanged(int percent);
    void warning(const QString &msg);
    void error(const QString &msg);
    void onlineChanged(bool state);
    void resourceNameChanged(const QString &name);

    void refreshAgentStatus();
    void refreshResourceStatus();

    void errorHandler(const QDBusError &error);

private:
    template <typename T> T *findInterface(Akonadi::DBus::AgentType agentType, const char *path = nullptr);

protected:
    void setAgentType(const QString &agentType)
    {
        mType = agentType;
    }

private:
    QString mIdentifier;
    QString mType;
    AgentManager *mManager = nullptr;
    org::freedesktop::Akonadi::Agent::Control *mAgentControlInterface = nullptr;
    org::freedesktop::Akonadi::Agent::Status *mAgentStatusInterface = nullptr;
    org::freedesktop::Akonadi::Agent::Search *mSearchInterface = nullptr;
    org::freedesktop::Akonadi::Resource *mResourceInterface = nullptr;
    org::freedesktop::Akonadi::Preprocessor *mPreprocessorInterface = nullptr;

    int mStatus = 0;
    QString mStatusMessage;
    int mPercent = 0;
    QString mResourceName;
    bool mOnline = false;
    bool mPendingQuit = false;

};

#endif
