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

#ifndef AKONADI_AGENTCONFIGURATIONFACTORYBASE_H
#define AKONADI_AGENTCONFIGURATIONFACTORYBASE_H

#include "akonadicore_export.h"

#include <QObject>
#include <KSharedConfig>

namespace Akonadi {

class AgentConfigurationBase;
class AKONADICORE_EXPORT AgentConfigurationFactoryBase : public QObject
{
    Q_OBJECT
public:
    explicit AgentConfigurationFactoryBase(QObject *parent = nullptr);
    ~AgentConfigurationFactoryBase() override = default;

    virtual AgentConfigurationBase *create(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args) const = 0;
};

}

#define AKONADI_AGENTCONFIG_FACTORY(FactoryName, metadata, ClassName) \
    class FactoryName : public Akonadi::AgentConfigurationFactoryBase { \
        Q_OBJECT \
        Q_PLUGIN_METADATA(IID "org.freedesktop.Akonadi.AgentConfig" FILE metadata) \
    public: \
        FactoryName(QObject *parent = nullptr): Akonadi::AgentConfigurationFactoryBase(parent) {} \
        Akonadi::AgentConfigurationBase *create(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args) const override { \
            return new ClassName(config, parent, args); \
        } \
    };

#endif
