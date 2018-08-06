/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include "agentconfigurationbase.h"
#include "akonadicore_debug.h"

#include <QDBusConnection>


namespace Akonadi {
class Q_DECL_HIDDEN AgentConfigurationBase::Private {
public:
    Private(KSharedConfigPtr config, const QVariantList &args)
        : config(config)
    {
        Q_ASSERT(args.size() >= 1);
        if (args.empty()) {
            qCCritical(AKONADICORE_LOG, "AgentConfigurationBase instantiated with invalid arguments");
            return;
        }
        identifier = args.at(0).toString();
    }

    KSharedConfigPtr config;
    QString identifier;
};
}

using namespace Akonadi;

AgentConfigurationBase::AgentConfigurationBase(KSharedConfigPtr config,
                                               QObject *parentWidget,
                                               const QVariantList &args)
    : QObject(parentWidget)
    , d(new Private(config, args))
{
    AgentConfigManager::self()->registerPlugin(d->identifier);
}

AgentConfigurationBase::~AgentConfigurationBase()
{
    AgentConfigManager::self()->unregisterPlugin(d->identifier);
}

KSharedConfigPtr AgentConfigurationBase::config() const
{
    return d->config;
}

QString AgentConfigurationBase::identifier() const
{
    return d->identifier;
}

void AgentConfigurationBase::load()
{
}

void AgentConfigurationBase::save() const
{
}
