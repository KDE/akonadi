/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AGENTTHREAD_H
#define AKONADI_AGENTTHREAD_H

#include <QThread>

namespace Akonadi
{

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
    AgentThread(const QString &identifier, QObject *factory, QObject *parent = nullptr);

    /**
     * Configures the agent.
     *
     * @param windowId The parent window id for the config dialog.
     */
    void configure(qlonglong windowId);

protected:
    void run() override;

private:
    const QString m_identifier;
    QObject *const m_factory;
    QObject *m_instance = nullptr;
};

}

#endif
