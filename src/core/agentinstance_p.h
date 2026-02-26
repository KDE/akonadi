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
 * \internal
 */
class AgentInstancePrivate : public QSharedData
{
public:
    AgentInstancePrivate()
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
    QString mAccountId;
};

}
