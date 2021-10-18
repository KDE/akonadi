/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QSharedData>
#include <QStringList>
#include <QVariantMap>

namespace Akonadi
{
/**
 * @internal
 */
class AgentTypePrivate : public QSharedData
{
public:
    AgentTypePrivate()
    {
    }

    AgentTypePrivate(const AgentTypePrivate &other)
        : QSharedData(other)
    {
        mIdentifier = other.mIdentifier;
        mName = other.mName;
        mDescription = other.mDescription;
        mIconName = other.mIconName;
        mMimeTypes = other.mMimeTypes;
        mCapabilities = other.mCapabilities;
        mCustomProperties = other.mCustomProperties;
    }

    QString mIdentifier;
    QString mName;
    QString mDescription;
    QString mIconName;
    QStringList mMimeTypes;
    QStringList mCapabilities;
    QVariantMap mCustomProperties;
};

}

