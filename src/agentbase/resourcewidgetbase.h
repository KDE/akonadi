/*
    This file is part of akonadiresources.

    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiagentwidgetbase_export.h"

#include <QApplication>
#include <akonadi/resourcebase.h>

namespace Akonadi
{

class AKONADIAGENTWIDGETBASE_EXPORT ResourceWidgetBase : public ResourceBase
{
public:
    /**
     * Use this method in the main function of your agent
     * application to initialize your agent subclass.
     * This method also takes care of creating a QApplication
     * object and parsing command line arguments.
     *
     * @note In case the given class is also derived from AgentBase::Observer
     *       it gets registered as its own observer (see AgentBase::Observer), e.g.
     *       <tt>agentInstance->registerObserver( agentInstance );</tt>
     *
     * @code
     *
     *   class MyAgent : public AgentBase
     *   {
     *     ...
     *   };
     *
     *   AKONADI_AGENT_MAIN( MyAgent )
     *
     * @endcode
     *
     * @param argc number of arguments
     * @param argv arguments for the function
     */
    template<typename T>
    static int init(int argc, char **argv)
    {
        // Disable session management
        qunsetenv("SESSION_MANAGER");

        QApplication app(argc, argv);
        debugAgent(argc, argv);
        const QString id = parseArguments(argc, argv);
        T r(id);

        // check if T also inherits AgentBase::Observer and
        // if it does, automatically register it on itself
        auto observer = dynamic_cast<Observer *>(&r);
        if (observer != nullptr) {
            r.registerObserver(observer);
        }
        r.initTheming();
        return AgentBase::init(r);
    }

protected:
    ResourceWidgetBase(const QString &id);

private:
    void initTheming();
};
}

#ifndef AKONADI_RESOURCE_MAIN
/**
 * Convenience Macro for the most common main() function for Akonadi resources.
 */
#define AKONADI_RESOURCE_MAIN(resourceClass)                                                                                                                   \
    int main(int argc, char **argv)                                                                                                                            \
    {                                                                                                                                                          \
        return Akonadi::ResourceWidgetBase::init<resourceClass>(argc, argv);                                                                                   \
    }
#endif
