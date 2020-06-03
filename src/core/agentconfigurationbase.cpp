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
#include "agentmanager.h"
#include "akonadicore_debug.h"


#include <KAboutData>
#include <QDialogButtonBox>
#include <QSize>

namespace Akonadi {
class Q_DECL_HIDDEN AgentConfigurationBase::Private {
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

AgentConfigurationBase::AgentConfigurationBase(const KSharedConfigPtr &config,
                                               QWidget *parentWidget,
                                               const QVariantList &args)
    : QObject(reinterpret_cast<QObject*>(parentWidget))
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

void AgentConfigurationBase::saveDialogSize(const QSize& /*unused*/) // clazy:exclude=function-args-by-value
{}

QDialogButtonBox::StandardButtons AgentConfigurationBase::standardButtons() const
{
    return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel;
}
