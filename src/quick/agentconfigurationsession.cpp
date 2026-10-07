// SPDX-FileCopyrightText: 2026 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "agentconfigurationsession.h"
#include "akonadi_quick_debug.h"

#include <Akonadi/AgentConfigurationQuickBase>
#include <Akonadi/AgentManager>
#include <Akonadi/ServerManager>
#include <core/agentconfigurationmanager_p.h>

using namespace Akonadi;

AgentConfigurationSession::AgentConfigurationSession(const AgentInstance &instance, QQmlEngine *engine, bool removeOnCancel, QObject *parent)
    : QObject(parent)
    , m_instance(instance)
{
    if (!engine) {
        return;
    }
    const auto path = AgentConfigurationManager::self()->findConfigPlugin(instance.type().identifier(), AgentConfigurationManager::Ui::Quick);
    if (path.isEmpty()) {
        return;
    }
    m_registered = AgentConfigurationManager::self()->registerInstanceConfiguration(instance.identifier());
    if (!m_registered) {
        qCWarning(AKONADI_QUICK_LOG) << "Configuration is already open for" << instance.identifier();
        return;
    }
    m_loader.setFileName(path);
    auto factory = qobject_cast<AgentConfigurationQuickFactory *>(m_loader.instance());
    if (!factory) {
        qCWarning(AKONADI_QUICK_LOG) << "Invalid Quick configuration plugin" << path << m_loader.errorString();
        return;
    }
    const QString configName = ServerManager::addNamespace(instance.identifier()) + QStringLiteral("rc");
    m_controller = factory->create(KSharedConfig::openConfig(configName), instance.identifier(), this);
    if (!m_controller) {
        return;
    }
    connect(m_controller, &AgentConfigurationQuickBase::enableOkButton, this, [this](bool enabled) {
        if (m_canSave != enabled) {
            m_canSave = enabled;
            Q_EMIT canSaveChanged();
        }
    });
    const auto page = factory->page();
    m_page = new QQmlComponent(engine, page.module, page.component, this);
    if (m_page->isError()) {
        qCWarning(AKONADI_QUICK_LOG) << "Unable to load agent configuration page" << m_page->errors();
        return;
    }
    m_controller->load();
    m_removeOnCancel = removeOnCancel;
}

AgentConfigurationSession::~AgentConfigurationSession()
{
    if (m_registered) {
        AgentConfigurationManager::self()->unregisterInstanceConfiguration(m_instance.identifier());
    }
    if (m_removeOnCancel && !m_finished) {
        AgentManager::self()->removeInstance(m_instance);
    }
    delete m_page;
    delete m_controller;
    m_loader.unload();
}

bool AgentConfigurationSession::isValid() const
{
    return m_registered && m_controller && m_page && m_page->isReady();
}

QQmlComponent *AgentConfigurationSession::page() const
{
    return m_page;
}

QObject *AgentConfigurationSession::controller() const
{
    return m_controller;
}

bool AgentConfigurationSession::canSave() const
{
    return m_canSave;
}

void AgentConfigurationSession::save()
{
    if (m_finished || !m_canSave || !m_controller || !m_controller->save()) {
        return;
    }
    m_instance.reconfigure();
    m_finished = true;
    Q_EMIT finished();
}

void AgentConfigurationSession::cancel()
{
    deleteLater();
}

#include "moc_agentconfigurationsession.cpp"
