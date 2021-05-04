/*
    This file is part of akonadiresources.

    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentbase.h"
#include "akonadiagentbase_export.h"

#include <QObject>

namespace Akonadi
{
class AgentFactoryBasePrivate;

/**
 * @short A factory base class for in-process agents.
 *
 * @see AKONADI_AGENT_FACTORY()
 * @internal
 * @since 4.6
 */
class AKONADIAGENTBASE_EXPORT AgentFactoryBase : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new agent factory.
     * Executed in the main thread, performs KDE infrastructure setup.
     *
     * @param catalogName The translation catalog of this resource.
     * @param parent The parent object.
     */
    explicit AgentFactoryBase(const char *catalogName, QObject *parent = nullptr);

    ~AgentFactoryBase() override;

public Q_SLOTS:
    /**
     * Creates a new agent instance with the given identifier.
     */
    virtual QObject *createInstance(const QString &identifier) const = 0;

protected:
    void createComponentData(const QString &identifier) const;

private:
    AgentFactoryBasePrivate *const d;
};

/**
 * @short A factory for in-process agents.
 *
 * @see AKONADI_AGENT_FACTORY()
 * @internal
 * @since 4.6
 */
template<typename T> class AgentFactory : public AgentFactoryBase
{
public:
    /** reimplemented */
    explicit AgentFactory(const char *catalogName, QObject *parent = nullptr)
        : AgentFactoryBase(catalogName, parent)
    {
    }

    QObject *createInstance(const QString &identifier) const override
    {
        createComponentData(identifier);
        T *instance = new T(identifier);

        // check if T also inherits AgentBase::Observer and
        // if it does, automatically register it on itself
        auto observer = dynamic_cast<Akonadi::AgentBase::Observer *>(instance);
        if (observer != nullptr) {
            instance->registerObserver(observer);
        }

        return instance;
    }
};

}

