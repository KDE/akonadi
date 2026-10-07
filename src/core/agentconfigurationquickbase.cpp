// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "agentconfigurationquickbase.h"

using namespace Akonadi;

AgentConfigurationQuickBase::AgentConfigurationQuickBase(const KSharedConfigPtr &config, const QString &identifier, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_identifier(identifier)
{
}

AgentConfigurationQuickBase::~AgentConfigurationQuickBase() = default;

QString AgentConfigurationQuickBase::identifier() const
{
    return m_identifier;
}

void AgentConfigurationQuickBase::load()
{
    m_config->reparseConfiguration();
}

bool AgentConfigurationQuickBase::save()
{
    m_config->sync();
    m_config->reparseConfiguration();
    return true;
}

KSharedConfigPtr AgentConfigurationQuickBase::config() const
{
    return m_config;
}

#include "moc_agentconfigurationquickbase.cpp"
