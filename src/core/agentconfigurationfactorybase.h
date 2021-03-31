/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <KSharedConfig>
#include <QObject>

namespace Akonadi
{
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

#define AKONADI_AGENTCONFIG_FACTORY(FactoryName, metadata, ClassName)                                                                                          \
    class FactoryName : public Akonadi::AgentConfigurationFactoryBase                                                                                          \
    {                                                                                                                                                          \
        Q_OBJECT                                                                                                                                               \
        Q_PLUGIN_METADATA(IID "org.freedesktop.Akonadi.AgentConfig" FILE metadata)                                                                             \
    public:                                                                                                                                                    \
        FactoryName(QObject *parent = nullptr)                                                                                                                 \
            : Akonadi::AgentConfigurationFactoryBase(parent)                                                                                                   \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        Akonadi::AgentConfigurationBase *create(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args) const override                      \
        {                                                                                                                                                      \
            return new ClassName(config, parent, args);                                                                                                        \
        }                                                                                                                                                      \
    };

