/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentconfigurationbase.h"
#include "akonadicore_debug.h"

#include <KConfigGroup>
#include <QSize>

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

QSize AgentConfigurationBase::restoreDialogSize() const
{
    return {};
}

void AgentConfigurationBase::saveDialogSize(const QSize & /*unused*/) // clazy:exclude=function-args-by-value
{
}

void AgentConfigurationBase::saveActivitiesSettings(const ActivitySettings &activities) const
{
    KConfigGroup activitiesGroup = d->config->group(QStringLiteral("Agent"));
    activitiesGroup.writeEntry(QStringLiteral("ActivitiesEnabled"), activities.enabled);
    activitiesGroup.writeEntry(QStringLiteral("Activities"), activities.activities);
}

AgentConfigurationBase::ActivitySettings AgentConfigurationBase::restoreActivitiesSettings() const
{
    AgentConfigurationBase::ActivitySettings settings;
    const KConfigGroup activitiesGroup = d->config->group(QStringLiteral("Agent"));
    settings.enabled = activitiesGroup.readEntry(QStringLiteral("ActivitiesEnabled"), false);
    settings.activities = activitiesGroup.readEntry(QStringLiteral("Activities"), QStringList());
    return settings;
}

#include "moc_agentconfigurationbase.cpp"
