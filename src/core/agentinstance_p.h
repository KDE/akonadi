/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agenttype.h"

#include <QSharedData>
#include <QString>

namespace Akonadi
{
/**
 * @internal
 */
class AgentInstancePrivate : public QSharedData
{
public:
    AgentInstancePrivate()
    {
    }

    AgentInstancePrivate(const AgentInstancePrivate &other)
        : QSharedData(other)
        , mType(other.mType)
        , mIdentifier(other.mIdentifier)
        , mName(other.mName)
        , mStatus(other.mStatus)
        , mStatusMessage(other.mStatusMessage)
        , mActivities(other.mActivities)
        , mProgress(other.mProgress)
        , mIsOnline(other.mIsOnline)
        , mActivitiesEnabled(other.mActivitiesEnabled)
    {
    }

    AgentType mType;
    QString mIdentifier;
    QString mName;
    int mStatus = 0;
    QString mStatusMessage;
    QStringList mActivities;
    int mProgress = 0;
    bool mIsOnline = false;
    bool mActivitiesEnabled = false;
};

}
