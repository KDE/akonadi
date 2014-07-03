/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTFACTORY_H
#define AKONADI_AGENTFACTORY_H

#include "akonadiagentbase_export.h"
#include "agentbase.h"

#include <QtCore/QObject>
#include <QtCore/QtPlugin>

namespace Akonadi {

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
    explicit AgentFactoryBase(const char *catalogName, QObject *parent = 0);

    virtual ~AgentFactoryBase();

public Q_SLOTS:
    /**
     * Creates a new agent instace with the given identifier.
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
template <typename T>
class AgentFactory : public AgentFactoryBase
{
public:
    /** reimplemented */
    explicit AgentFactory(const char *catalogName, QObject *parent = 0)
        : AgentFactoryBase(catalogName, parent)
    {
    }

    QObject *createInstance(const QString &identifier) const
    {
        createComponentData(identifier);
        T *instance = new T(identifier);

        // check if T also inherits AgentBase::Observer and
        // if it does, automatically register it on itself
        Akonadi::AgentBase::Observer *observer = dynamic_cast<Akonadi::AgentBase::Observer *>(instance);
        if (observer != 0) {
            instance->registerObserver(observer);
        }

        return instance;
    }
};

}

#endif
