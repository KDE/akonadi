/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentconfigurationwidget.h"
#include "agentconfigurationdialog.h"
#include "agentconfigurationwidget_p.h"
#include "akonadiwidgets_debug.h"
#include "core/agentconfigurationbase.h"
#include "core/agentconfigurationfactorybase.h"
#include "core/agentconfigurationmanager_p.h"
#include "core/agentmanager.h"
#include "core/servermanager.h"

#include <QChildEvent>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KSharedConfig>

#include <memory>

using namespace Akonadi;
using namespace std::chrono_literals;
AgentConfigurationWidgetPrivate::AgentConfigurationWidgetPrivate(const AgentInstance &instance)
    : agentInstance(instance)
{
}

AgentConfigurationWidgetPrivate::~AgentConfigurationWidgetPrivate()
{
}

void AgentConfigurationWidgetPrivate::setupErrorWidget(QWidget *parent, const QString &text)
{
    auto layout = new QVBoxLayout(parent);
    layout->addStretch(2);
    auto label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    layout->addStretch(2);
}

bool AgentConfigurationWidgetPrivate::loadPlugin(const QString &pluginPath)
{
    if (pluginPath.isEmpty()) {
        qCDebug(AKONADIWIDGETS_LOG) << "Haven't found config plugin for" << agentInstance.type().identifier();
        return false;
    }
    loader = decltype(loader)(new QPluginLoader(pluginPath));
    if (!loader->load()) {
        qCWarning(AKONADIWIDGETS_LOG) << "Failed to load config plugin" << pluginPath << ":" << loader->errorString();
        loader.reset();
        return false;
    }
    factory = qobject_cast<AgentConfigurationFactoryBase *>(loader->instance());
    if (!factory) {
        // will unload the QPluginLoader and thus delete the factory as well
        qCWarning(AKONADIWIDGETS_LOG) << "Config plugin" << pluginPath << "does not contain AgentConfigurationFactory!";
        loader.reset();
        return false;
    }

    qCDebug(AKONADIWIDGETS_LOG) << "Loaded agent configuration plugin" << pluginPath;
    return true;
}

AgentConfigurationWidget::AgentConfigurationWidget(const AgentInstance &instance, QWidget *parent)
    : QWidget(parent)
    , d(new AgentConfigurationWidgetPrivate(instance))
{
    if (AgentConfigurationManager::self()->registerInstanceConfiguration(instance.identifier())) {
        const auto pluginPath = AgentConfigurationManager::self()->findConfigPlugin(instance.type().identifier());
        if (d->loadPlugin(pluginPath)) {
            QString configName = instance.identifier() + QStringLiteral("rc");
            configName = Akonadi::ServerManager::addNamespace(configName);
            KSharedConfigPtr config = KSharedConfig::openConfig(configName);
            auto layout = new QVBoxLayout(this);
            layout->setContentsMargins({});
            d->plugin = d->factory->create(config, this, {instance.identifier()});
            connect(d->plugin.data(), &AgentConfigurationBase::enableOkButton, this, &AgentConfigurationWidget::enableOkButton);
        } else {
            // Hide this dialog and fallback to calling the out-of-process configuration
            if (auto dlg = qobject_cast<AgentConfigurationDialog *>(parent)) {
                const_cast<AgentInstance &>(instance).configure(topLevelWidget()->parentWidget());
                // If we are inside the AgentConfigurationDialog, hide the dialog
                QTimer::singleShot(0s, this, [dlg]() {
                    dlg->reject();
                });
            } else {
                const_cast<AgentInstance &>(instance).configure();
                // Otherwise show a message that this is opened externally
                d->setupErrorWidget(this, i18n("The configuration dialog has been opened in another window"));
            }

            // TODO: Re-enable once we can kill the fallback code above ^^
            // d->setupErrorWidget(this, i18n("Failed to load configuration plugin"));
        }
    } else if (AgentConfigurationManager::self()->isInstanceRegistered(instance.identifier())) {
        d->setupErrorWidget(this, i18n("Configuration for %1 is already opened elsewhere.", instance.name()));
    } else {
        d->setupErrorWidget(this, i18n("Failed to register %1 configuration dialog.", instance.name()));
    }

    QTimer::singleShot(0, this, &AgentConfigurationWidget::load);
}

AgentConfigurationWidget::~AgentConfigurationWidget()
{
    AgentConfigurationManager::self()->unregisterInstanceConfiguration(d->agentInstance.identifier());
}

void AgentConfigurationWidget::load()
{
    if (d->plugin) {
        d->plugin->load();
    }
}

void AgentConfigurationWidget::save()
{
    qCDebug(AKONADIWIDGETS_LOG) << "Saving configuration for" << d->agentInstance.identifier();
    if (d->plugin) {
        if (d->plugin->save()) {
            d->agentInstance.reconfigure();
        }
    }
}

QSize AgentConfigurationWidget::restoreDialogSize() const
{
    if (d->plugin) {
        return d->plugin->restoreDialogSize();
    }
    return {};
}

void AgentConfigurationWidget::saveDialogSize(QSize size)
{
    if (d->plugin) {
        d->plugin->saveDialogSize(size);
    }
}

QDialogButtonBox::StandardButtons AgentConfigurationWidget::standardButtons() const
{
    if (d->plugin) {
        return d->plugin->standardButtons();
    }
    return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel;
}

void AgentConfigurationWidget::childEvent(QChildEvent *event)
{
    if (event->added()) {
        if (event->child()->isWidgetType()) {
            layout()->addWidget(static_cast<QWidget *>(event->child()));
        }
    }

    QWidget::childEvent(event);
}
