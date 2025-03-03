/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agenttype.h"
#include "agenttype_p.h"

#include <QIcon>

using namespace Akonadi;

AgentType::AgentType()
    : d(new AgentTypePrivate)
{
}

AgentType::AgentType(const AgentType &other)
    : d(other.d)
{
}

AgentType::~AgentType()
{
}

bool AgentType::isValid() const
{
    return !d->mIdentifier.isEmpty();
}

QString AgentType::identifier() const
{
    return d->mIdentifier;
}

QString AgentType::name() const
{
    return d->mName;
}

QString AgentType::description() const
{
    return d->mDescription;
}

QString AgentType::iconName() const
{
    return d->mIconName;
}

QIcon AgentType::icon() const
{
    return QIcon::fromTheme(d->mIconName);
}

QStringList AgentType::mimeTypes() const
{
    return d->mMimeTypes;
}

QStringList AgentType::capabilities() const
{
    return d->mCapabilities;
}

QVariantMap AgentType::customProperties() const
{
    return d->mCustomProperties;
}

AgentType &AgentType::operator=(const AgentType &other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool AgentType::operator==(const AgentType &other) const
{
    return (d->mIdentifier == other.d->mIdentifier);
}

size_t Akonadi::qHash(const Akonadi::AgentType &type, size_t seed) noexcept
{
    return ::qHash(type.identifier(), seed);
}
