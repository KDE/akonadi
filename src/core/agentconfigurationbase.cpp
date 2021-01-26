/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentconfigurationbase.h"
#include "agentmanager.h"
#include "akonadicore_debug.h"

#include <KAboutData>
#include <QSize>

namespace Akonadi
{
class Q_DECL_HIDDEN AgentConfigurationBase::Private
{
public:
    Private(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args)
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
    QScopedPointer<KAboutData> aboutData;
    QWidget *parentWidget = nullptr;
};
} // namespace Akonadi

using namespace Akonadi;

AgentConfigurationBase::AgentConfigurationBase(const KSharedConfigPtr &config, QWidget *parentWidget, const QVariantList &args)
    : QObject(reinterpret_cast<QObject *>(parentWidget))
    , d(new Private(config, parentWidget, args))
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

void AgentConfigurationBase::setKAboutData(const KAboutData &aboutData)
{
    d->aboutData.reset(new KAboutData(aboutData));
}

KAboutData *AgentConfigurationBase::aboutData() const
{
    return d->aboutData.data();
}

QSize AgentConfigurationBase::restoreDialogSize() const
{
    return {};
}

void AgentConfigurationBase::saveDialogSize(const QSize & /*unused*/) // clazy:exclude=function-args-by-value
{
}

QDialogButtonBox::StandardButtons AgentConfigurationBase::standardButtons() const
{
    return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel;
}
