/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AGENTINSTANCE_P_H
#define AKONADI_AGENTINSTANCE_P_H

#include "agenttype.h"

#include <QSharedData>
#include <QString>

namespace Akonadi
{

/**
 * @internal
 */
class Q_DECL_HIDDEN AgentInstance::Private : public QSharedData
{
public:
    Private()
        : mStatus(0)
        , mProgress(0)
        , mIsOnline(false)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
    {
        mType = other.mType;
        mIdentifier = other.mIdentifier;
        mName = other.mName;
        mStatus = other.mStatus;
        mStatusMessage = other.mStatusMessage;
        mProgress = other.mProgress;
        mIsOnline = other.mIsOnline;
    }

    AgentType mType;
    QString mIdentifier;
    QString mName;
    int mStatus;
    QString mStatusMessage;
    int mProgress;
    bool mIsOnline;
};

}

#endif
