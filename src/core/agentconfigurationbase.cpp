/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentconfigurationbase.h"
#include "agentinstance.h"
#include "agentmanager.h"
#include "akonadicore_debug.h"

#include <QVariant>

namespace Akonadi
{
class AgentConfigurationBasePrivate
{
public:
    AgentConfigurationBasePrivate(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args)
        : config(config)
        , parentWidget(parentWidget)
    {
        Q_ASSERT(!args.empty());
        if (args.empty()) {
            qCCritical(AKONADICORE_LOG, "AgentConfigurationBase instantiated with invalid arguments");
            return;
        }
        identifier = args.at(0).toString();
    }

    KSharedConfigPtr config;
    QString identifier;
    QWidget *const parentWidget;
};
} // namespace Akonadi

using namespace Akonadi;

AgentConfigurationBase::AgentConfigurationBase(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args)
    : QObject(reinterpret_cast<QObject *>(parentWidget))
    , d(new AgentConfigurationBasePrivate(config, parentWidget, args))
{
}

AgentConfigurationBase::~AgentConfigurationBase()
{
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
    d->config->reparseConfiguration();
}

bool AgentConfigurationBase::save() const
{
    d->config->sync();
    d->config->reparseConfiguration();
    return true;
}

QWidget *AgentConfigurationBase::parentWidget() const
{
    return d->parentWidget;
}

void AgentConfigurationBase::saveActivitiesSettings(const ActivitySettings &activities) const
{
    AgentInstance instance = AgentManager::self()->instance(identifier());
    if (!instance.isValid()) {
        return;
    }
    instance.setActivitiesEnabled(activities.enabled);
    instance.setActivities(activities.activities);
}

AgentConfigurationBase::ActivitySettings AgentConfigurationBase::restoreActivitiesSettings() const
{
    ActivitySettings settings;
    const AgentInstance instance = AgentManager::self()->instance(identifier());
    if (instance.isValid()) {
        settings.enabled = instance.activitiesEnabled();
        settings.activities = instance.activities();
    }
    return settings;
}

#include "moc_agentconfigurationbase.cpp"
