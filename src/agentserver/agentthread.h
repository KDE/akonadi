/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTTHREAD_H
#define AKONADI_AGENTTHREAD_H

#include <QtCore/QThread>

namespace Akonadi {

/**
 * @short A class that encapsulates an agent instance inside a thread.
 */
class AgentThread : public QThread
{
    Q_OBJECT

public:
    /**
     * Creates a new agent thread.
     *
     * @param identifier The unique identifier for this agent
     * @param factory The factory object that creates the agent instance.
     * @param parent The parent object.
     */
    AgentThread(const QString &identifier, QObject *factory, QObject *parent = Q_NULLPTR);

    /**
     * Configures the agent.
     *
     * @param windowId The parent window id for the config dialog.
     */
    void configure(qlonglong windowId);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    QString m_identifier;
    QObject *m_factory;
    QObject *m_instance;
};

}

#endif
