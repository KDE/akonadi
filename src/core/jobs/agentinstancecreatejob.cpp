/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstancecreatejob.h"

#include "agentinstance.h"
#include "agentmanager.h"
#include "agentmanager_p.h"
#include "controlinterface.h"
#include <QDBusConnection>
#include "kjobprivatebase_p.h"
#include "servermanager.h"

#include <KLocalizedString>

#include <QTimer>

#ifdef Q_OS_UNIX
#include <sys/types.h>
#include <signal.h>
#endif

using namespace Akonadi;

static const int safetyTimeout = 10000; // ms

namespace Akonadi
{
/**
 * @internal
 */
class AgentInstanceCreateJobPrivate : public KJobPrivateBase
{
    Q_OBJECT
public:
    explicit AgentInstanceCreateJobPrivate(AgentInstanceCreateJob *parent)
        : q(parent)
        , parentWidget(nullptr)
        , safetyTimer(new QTimer(parent))
        , doConfig(false)
        , tooLate(false)
    {
        connect(AgentManager::self(), &AgentManager::instanceAdded, this, &AgentInstanceCreateJobPrivate::agentInstanceAdded);
        connect(safetyTimer, &QTimer::timeout, this, &AgentInstanceCreateJobPrivate::timeout);
    }

    void agentInstanceAdded(const AgentInstance &instance) const
    {
        if (agentInstance == instance && !tooLate) {
            safetyTimer->stop();
            if (doConfig) {
                // return from dbus call first before doing the next one
                QTimer::singleShot(0, this, &AgentInstanceCreateJobPrivate::doConfigure);
            } else {
                q->emitResult();
            }
        }
    }

    void doConfigure()
    {
        org::freedesktop::Akonadi::Agent::Control *agentControlIface =
            new org::freedesktop::Akonadi::Agent::Control(ServerManager::agentServiceName(ServerManager::Agent, agentInstance.identifier()),
                    QStringLiteral("/"), QDBusConnection::sessionBus(), q);
        if (!agentControlIface || !agentControlIface->isValid()) {
            delete agentControlIface;

            q->setError(KJob::UserDefinedError);
            q->setErrorText(i18n("Unable to access D-Bus interface of created agent."));
            q->emitResult();
            return;
        }

        connect(agentControlIface, &org::freedesktop::Akonadi::Agent::Control::configurationDialogAccepted,
                this, [agentControlIface, this]() {
                    agentControlIface->deleteLater();
                    q->emitResult();
                });
        connect(agentControlIface, &org::freedesktop::Akonadi::Agent::Control::configurationDialogRejected,
                this, [agentControlIface, this]() {
                    agentControlIface->deleteLater();
                    AgentManager::self()->removeInstance(agentInstance);
                    q->emitResult();
                });

        agentInstance.configure(parentWidget);
    }

    void timeout()
    {
        tooLate = true;
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Agent instance creation timed out."));
        q->emitResult();
    }

    void doStart() override;

    AgentInstanceCreateJob * const q;
    AgentType agentType;
    QString agentTypeId;
    AgentInstance agentInstance;
    QWidget *parentWidget = nullptr;
    QTimer *safetyTimer = nullptr;
    bool doConfig;
    bool tooLate;
};

} // namespace Akonadi

AgentInstanceCreateJob::AgentInstanceCreateJob(const AgentType &agentType, QObject *parent)
    : KJob(parent)
    , d(new AgentInstanceCreateJobPrivate(this))
{
    d->agentType = agentType;
}

AgentInstanceCreateJob::AgentInstanceCreateJob(const QString &typeId, QObject *parent)
    : KJob(parent)
    , d(new AgentInstanceCreateJobPrivate(this))
{
    d->agentTypeId = typeId;
}

AgentInstanceCreateJob::~AgentInstanceCreateJob()
{
    delete d;
}

void AgentInstanceCreateJob::configure(QWidget *parent)
{
    d->parentWidget = parent;
    d->doConfig = true;
}

AgentInstance AgentInstanceCreateJob::instance() const
{
    return d->agentInstance;
}

void AgentInstanceCreateJob::start()
{
    d->start();
}

void AgentInstanceCreateJobPrivate::doStart()
{
    if (!agentType.isValid() && !agentTypeId.isEmpty()) {
        agentType = AgentManager::self()->type(agentTypeId);
    }

    if (!agentType.isValid()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to obtain agent type '%1'.", agentTypeId));
        QTimer::singleShot(0, q, &AgentInstanceCreateJob::emitResult);
        return;
    }

    agentInstance = AgentManager::self()->d->createInstance(agentType);
    if (!agentInstance.isValid()) {
        q->setError(KJob::UserDefinedError);
        q->setErrorText(i18n("Unable to create agent instance."));
        QTimer::singleShot(0, q, &AgentInstanceCreateJob::emitResult);
    } else {
        int timeout = safetyTimeout;
#ifdef Q_OS_UNIX
        // Increate the timeout when valgrinding the agent, because that slows down things a log.
        QString agentValgrind = QString::fromLocal8Bit(qgetenv("AKONADI_VALGRIND"));
        if (!agentValgrind.isEmpty() && agentType.identifier().contains(agentValgrind)) {
            timeout *= 15;
        }
#endif
        // change the timeout when debugging the agent, because we need time to start the debugger
        const QString agentDebugging = QString::fromLocal8Bit(qgetenv("AKONADI_DEBUG_WAIT"));
        if (!agentDebugging.isEmpty()) {
            // we are debugging
            const QString agentDebuggingTimeout = QString::fromLocal8Bit(qgetenv("AKONADI_DEBUG_TIMEOUT"));
            if (agentDebuggingTimeout.isEmpty()) {
                // use default value of 150 seconds (the same as "valgrinding", this has to be checked)
                timeout = 15 * safetyTimeout;
            } else {
                // use own value
                timeout = agentDebuggingTimeout.toInt();
            }
        }
        safetyTimer->start(timeout);
    }
}

#include "agentinstancecreatejob.moc"
