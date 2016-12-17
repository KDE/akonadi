/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "firstrun_p.h"
#include "KDBusConnectionPool"
#include "servermanager.h"

#include "agentinstance.h"
#include "agentinstancecreatejob.h"
#include "agentmanager.h"
#include "agenttype.h"

#include "akonadicore_debug.h"

#include <KConfig>
#include <KConfigGroup>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QMetaMethod>
#include <QMetaObject>
#include <QStandardPaths>
#include <QCoreApplication>

static const char FIRSTRUN_DBUSLOCK[] = "org.kde.Akonadi.Firstrun.lock";

using namespace Akonadi;

Firstrun::Firstrun(QObject *parent)
    : QObject(parent)
    , mConfig(new KConfig(ServerManager::addNamespace(QStringLiteral("akonadi-firstrunrc"))))
    , mCurrentDefault(0)
    , mProcess(0)
{
    //The code in firstrun is not safe in multi-instance mode
    Q_ASSERT(!ServerManager::hasInstanceIdentifier());
    if (ServerManager::hasInstanceIdentifier()) {
        deleteLater();
        return;
    }
    qCDebug(AKONADICORE_LOG);
    if (KDBusConnectionPool::threadConnection().registerService(QLatin1String(FIRSTRUN_DBUSLOCK))) {
        findPendingDefaults();
        qCDebug(AKONADICORE_LOG) << mPendingDefaults;
        setupNext();
    } else {
        qCDebug(AKONADICORE_LOG) << "D-Bus lock found, so someone else does the work for us already.";
        deleteLater();
    }
}

Firstrun::~Firstrun()
{
    if (qApp) {
        KDBusConnectionPool::threadConnection().unregisterService(QLatin1String(FIRSTRUN_DBUSLOCK));
    }
    delete mConfig;
    qCDebug(AKONADICORE_LOG) << "done";
}

void Firstrun::findPendingDefaults()
{
    const KConfigGroup cfg(mConfig, "ProcessedDefaults");
    const QStringList paths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi/firstrun"), QStandardPaths::LocateDirectory);
    for (const QString &dirName : paths) {
        const QStringList files = QDir(dirName).entryList(QDir::Files | QDir::Readable);
        for (const QString &fileName : files) {
            const QString fullName = dirName + QLatin1Char('/') + fileName;
            KConfig c(fullName);
            const QString id = KConfigGroup(&c, "Agent").readEntry("Id", QString());
            if (id.isEmpty()) {
                qCWarning(AKONADICORE_LOG) << "Found invalid default configuration in " << fullName;
                continue;
            }
            if (cfg.hasKey(id)) {
                continue;
            }
            mPendingDefaults << fullName;
        }
    }

}

void Firstrun::setupNext()
{
    delete mCurrentDefault;
    mCurrentDefault = 0;

    if (mPendingDefaults.isEmpty()) {
        deleteLater();
        return;
    }

    mCurrentDefault = new KConfig(mPendingDefaults.takeFirst());
    const KConfigGroup agentCfg = KConfigGroup(mCurrentDefault, "Agent");

    AgentType type = AgentManager::self()->type(agentCfg.readEntry("Type", QString()));
    if (!type.isValid()) {
        qCCritical(AKONADICORE_LOG) << "Unable to obtain agent type for default resource agent configuration " << mCurrentDefault->name();
        setupNext();
        return;
    }
    if (type.capabilities().contains(QStringLiteral("Unique"))) {
        Q_FOREACH (const AgentInstance &agent, AgentManager::self()->instances()) {
            if (agent.type() == type) {
                // remember we set this one up already
                KConfigGroup cfg(mConfig, "ProcessedDefaults");
                cfg.writeEntry(agentCfg.readEntry("Id", QString()), agent.identifier());
                cfg.sync();
                setupNext();
                return;
            }
        }
    }

    AgentInstanceCreateJob *job = new AgentInstanceCreateJob(type);
    connect(job, &AgentInstanceCreateJob::result, this, &Firstrun::instanceCreated);
    job->start();
}

void Firstrun::instanceCreated(KJob *job)
{
    Q_ASSERT(mCurrentDefault);

    if (job->error()) {
        qCCritical(AKONADICORE_LOG) << "Creating agent instance failed for " << mCurrentDefault->name();
        setupNext();
        return;
    }

    AgentInstance instance = static_cast<AgentInstanceCreateJob *>(job)->instance();
    const KConfigGroup agentCfg = KConfigGroup(mCurrentDefault, "Agent");
    const QString agentName = agentCfg.readEntry("Name", QString());
    if (!agentName.isEmpty()) {
        instance.setName(agentName);
    }


    QDBusInterface *iface = new QDBusInterface(QStringLiteral("org.freedesktop.Akonadi.Agent.%1").arg(instance.identifier()),
            QStringLiteral("/Settings"), QString(),
            KDBusConnectionPool::threadConnection(), this);
    if (!iface->isValid()) {
        qCCritical(AKONADICORE_LOG) << "Unable to obtain the KConfigXT D-Bus interface of " << instance.identifier();
        setupNext();
        delete iface;
        return;
    }
    // agent specific settings, using the D-Bus <-> KConfigXT bridge
    const KConfigGroup settings = KConfigGroup(mCurrentDefault, "Settings");

    foreach (const QString &setting, settings.keyList()) {
        qCDebug(AKONADICORE_LOG) << "Setting up " << setting << " for agent " << instance.identifier();
        const QString methodName = QStringLiteral("set%1").arg(setting);
        const QVariant::Type argType = argumentType(iface->metaObject(), methodName);
        if (argType == QVariant::Invalid) {
            qCCritical(AKONADICORE_LOG) << "Setting " << setting << " not found in agent configuration interface of " << instance.identifier();
            continue;
        }

        QVariant arg;
        if (argType == QVariant::String) {
            // Since a string could be a path we always use readPathEntry here,
            // that shouldn't harm any normal string settings
            arg = settings.readPathEntry(setting, QString());
        } else {
            arg = settings.readEntry(setting, QVariant(argType));
        }

        const QDBusReply<void> reply = iface->call(methodName, arg);
        if (!reply.isValid()) {
            qCCritical(AKONADICORE_LOG) << "Setting " << setting << " failed for agent " << instance.identifier();
        }
    }

    iface->call(QStringLiteral("writeConfig"));

    instance.reconfigure();
    instance.synchronize();
    delete iface;

    // remember we set this one up already
    KConfigGroup cfg(mConfig, "ProcessedDefaults");
    cfg.writeEntry(agentCfg.readEntry("Id", QString()), instance.identifier());
    cfg.sync();

    setupNext();
}

QVariant::Type Firstrun::argumentType(const QMetaObject *mo, const QString &method)
{
    QMetaMethod m;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QString signature = QString::fromLatin1(mo->method(i).methodSignature());
        if (signature.startsWith(method)) {
            m = mo->method(i);
        }
    }

    if (m.methodSignature().isEmpty()) {
        return QVariant::Invalid;
    }

    const QList<QByteArray> argTypes = m.parameterTypes();
    if (argTypes.count() != 1) {
        return QVariant::Invalid;
    }

    return QVariant::nameToType(argTypes.first().constData());
}

#include "moc_firstrun_p.cpp"
