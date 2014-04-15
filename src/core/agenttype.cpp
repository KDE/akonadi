/*
    Copyright (c) 2008 Tobias Koenig <tokoe@kde.org>

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

#include "agenttype.h"
#include "agenttype_p.h"

#include <QIcon>

using namespace Akonadi;

AgentType::AgentType()
    : d(new Private)
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
