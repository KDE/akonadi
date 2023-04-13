/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentwidgetconfigurationbase.h"
#include "akonadiwidgets_debug.h"

#include <KAboutData>
#include <QSize>

using namespace Akonadi;

namespace Akonadi
{

class AgentWidgetConfigurationBasePrivate
{
public:
    QWidget *parentWidget = nullptr;
};

} // namespace Akonadi

AgentWidgetConfigurationBase::AgentWidgetConfigurationBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : AgentConfigurationBase(config, parent, args)
    , d(std::make_unique<AgentWidgetConfigurationBasePrivate>())
{
    d->parentWidget = parent;
}

AgentWidgetConfigurationBase::~AgentWidgetConfigurationBase() = default;

QWidget *AgentWidgetConfigurationBase::parentWidget() const
{
    return d->parentWidget;
}

QSize AgentWidgetConfigurationBase::restoreDialogSize() const
{
    return {};
}

void AgentWidgetConfigurationBase::saveDialogSize(const QSize & /*unused*/) // clazy:exclude=function-args-by-value
{
}

QDialogButtonBox::StandardButtons AgentWidgetConfigurationBase::standardButtons() const
{
    return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel;
}
