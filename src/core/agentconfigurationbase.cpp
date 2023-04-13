/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentconfigurationbase.h"
#include "akonadicore_debug.h"

#include <KAboutData>
#include <QSize>

namespace Akonadi
{
class AgentConfigurationBasePrivate
{
public:
    AgentConfigurationBasePrivate(const KSharedConfigPtr &config, const QVariantList &args)
        : config(config)
    {
        if (args.size() != 2) {
            qCCritical(AKONADICORE_LOG, "AgentConfigurationBase instantiated with invalid number of arguments (expected 2, got %d)", args.size());
            return;
        }
        identifier = args.at(0).toString();
        agentType = args.at(1).toString();
    }

    KSharedConfigPtr config;
    QString identifier;
    QString agentType;
    QScopedPointer<KAboutData> aboutData;
};
} // namespace Akonadi

using namespace Akonadi;

AgentConfigurationBase::AgentConfigurationBase(const KSharedConfigPtr &config, QObject *parent, const QVariantList &args)
    : QObject(parent)
    , d(new AgentConfigurationBasePrivate(config, args))
{
}

AgentConfigurationBase::~AgentConfigurationBase() = default;

KSharedConfigPtr AgentConfigurationBase::config() const
{
    return d->config;
}

QString AgentConfigurationBase::identifier() const
{
    return d->identifier;
}

QString AgentConfigurationBase::agentType() const
{
    return d->agentType;
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

void AgentConfigurationBase::setKAboutData(const KAboutData &aboutData)
{
    d->aboutData.reset(new KAboutData(aboutData));
}

KAboutData *AgentConfigurationBase::aboutData() const
{
    return d->aboutData.data();
}
