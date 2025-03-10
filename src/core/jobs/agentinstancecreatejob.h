/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agenttype.h"
#include "akonadicore_export.h"

#include <KJob>

#include <memory>

namespace Akonadi
{
class AgentInstance;
class AgentInstanceCreateJobPrivate;

/**
 * @short Job for creating new agent instances.
 *
 * This class encapsulates the procedure of creating a new agent instance
 * and optionally configuring it immediately.
 *
 * Example:
 *
 * @code
 * using namespace Qt::StringLiterals;
 *
 * MyClass::MyClass(QWidget *parent)
 *   : QWidget(parent)
 * {
 *     // Get agent type object
 *     Akonadi::AgentType type = Akonadi::AgentManager::self()->type(u"akonadi_vcard_resource"_s);
 *
 *     auto job = new Akonadi::AgentInstanceCreateJob(type);
 *     connect(job, &KJob::result, this, &MyClass::slotCreated);
 *
 *     // use this widget as parent for the config dialog
 *     job->configure(this);
 *
 *     job->start();
 * }
 *
 * ...
 *
 * void MyClass::slotCreated(KJob *job)
 * {
 *     auto createJob = static_cast<Akonadi::AgentInstanceCreateJob*>(job);
 *
 *     qDebug() << "Created agent instance:" << createJob->instance().identifier();
 * }
 * @endcode
 *
 * @note This is a KJob not an Akonadi::Job, so it won't auto-start!
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT AgentInstanceCreateJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Creates a new agent instance create job.
     *
     * @param type The type of the agent to create.
     * @param parent The parent object.
     */
    explicit AgentInstanceCreateJob(const AgentType &type, QObject *parent = nullptr);

    /**
     * Creates a new agent instance create job.
     *
     * @param typeId The identifier of type of the agent to create.
     * @param parent The parent object.
     * @since 4.5
     */
    explicit AgentInstanceCreateJob(const QString &typeId, QObject *parent = nullptr);

    /**
     * Destroys the agent instance create job.
     */
    ~AgentInstanceCreateJob() override;

    /**
     * Setup the job to show agent configuration dialog once the agent instance
     * has been successfully started.
     * @param parent The parent window for the configuration dialog.
     * @deprecated Use the new Akonadi::AgentConfigurationWidget and
     * Akonadi::AgentConfigurationDialog to display configuration dialogs
     * in-process
     */
    void configure(QWidget *parent = nullptr);

    /**
     * Returns the AgentInstance object of the newly created agent instance.
     */
    [[nodiscard]] AgentInstance instance() const;

    /**
     * Starts the instance creation.
     */
    void start() override;

private:
    /// @cond PRIVATE
    friend class Akonadi::AgentInstanceCreateJobPrivate;
    std::unique_ptr<AgentInstanceCreateJobPrivate> const d;
    /// @endcond
};

}
