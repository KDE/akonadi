/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "firstrun_p.h"
#include "servermanager.h"
#include <QDBusConnection>

#include "agentinstance.h"
#include "agentinstancecreatejob.h"
#include "agentmanager.h"
#include "agenttype.h"
#include <private/standarddirs_p.h>

#include "akonadicore_debug.h"

#include <KConfig>
#include <KConfigGroup>

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QMetaMethod>
#include <QMetaObject>
#include <QStandardPaths>

static const char FIRSTRUN_DBUSLOCK[] = "org.kde.Akonadi.Firstrun.lock";

using namespace Akonadi;

Firstrun::Firstrun(QObject *parent)
    : QObject(parent)
    , mConfig(new KConfig(ServerManager::addNamespace(QStringLiteral("akonadi-firstrunrc"))))
{
    // The code in firstrun is not safe in multi-instance mode
    Q_ASSERT(!ServerManager::hasInstanceIdentifier());
    if (ServerManager::hasInstanceIdentifier()) {
        deleteLater();
        return;
    }
    if (QDBusConnection::sessionBus().registerService(QLatin1String(FIRSTRUN_DBUSLOCK))) {
        findPendingDefaults();
        qCDebug(AKONADICORE_LOG) << "D-Bus lock acquired, pending defaults:" << mPendingDefaults;
        setupNext();
    } else {
        qCDebug(AKONADICORE_LOG) << "D-Bus lock found, so someone else does the work for us already.";
        deleteLater();
    }
}

Firstrun::~Firstrun()
{
    if (qApp) {
        QDBusConnection::sessionBus().unregisterService(QLatin1String(FIRSTRUN_DBUSLOCK));
    }
    delete mConfig;
    qCDebug(AKONADICORE_LOG) << "done";
}

void Firstrun::findPendingDefaults()
{
    const KConfigGroup cfg(mConfig, QLatin1String("ProcessedDefaults"));
    const auto paths = StandardDirs::locateAllResourceDirs(QStringLiteral("akonadi/firstrun"));
    for (const QString &dirName : paths) {
        const QStringList files = QDir(dirName).entryList(QDir::Files | QDir::Readable);
        for (const QString &fileName : files) {
            const QString fullName = dirName + QLatin1Char('/') + fileName;
            KConfig c(fullName);
            const QString id = KConfigGroup(&c, QLatin1String("Agent")).readEntry("Id", QString());
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
    mCurrentDefault = nullptr;

    if (mPendingDefaults.isEmpty()) {
        deleteLater();
        return;
    }

    mCurrentDefault = new KConfig(mPendingDefaults.takeFirst());
    const KConfigGroup agentCfg = KConfigGroup(mCurrentDefault, QLatin1String("Agent"));

    AgentType type = AgentManager::self()->type(agentCfg.readEntry("Type", QString()));
    if (!type.isValid()) {
        qCCritical(AKONADICORE_LOG) << "Unable to obtain agent type for default resource agent configuration " << mCurrentDefault->name();
        setupNext();
        return;
    }
    if (type.capabilities().contains(QLatin1String("Unique"))) {
        const Akonadi::AgentInstance::List lstAgents = AgentManager::self()->instances();
        for (const AgentInstance &agent : lstAgents) {
            if (agent.type() == type) {
                // remember we set this one up already
                KConfigGroup cfg(mConfig, QLatin1String("ProcessedDefaults"));
                cfg.writeEntry(agentCfg.readEntry("Id", QString()), agent.identifier());
                cfg.sync();
                setupNext();
                return;
            }
        }
    }

    auto job = new AgentInstanceCreateJob(type);
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
    const KConfigGroup agentCfg = KConfigGroup(mCurrentDefault, QLatin1String("Agent"));
    const QString agentName = agentCfg.readEntry("Name", QString());
    if (!agentName.isEmpty()) {
        instance.setName(agentName);
    }

    const auto service = ServerManager::agentServiceName(ServerManager::Agent, instance.identifier());
    auto iface = new QDBusInterface(service, QStringLiteral("/Settings"), QString(), QDBusConnection::sessionBus(), this);
    if (!iface->isValid()) {
        qCCritical(AKONADICORE_LOG) << "Unable to obtain the KConfigXT D-Bus interface of " << instance.identifier();
        setupNext();
        delete iface;
        return;
    }
    // agent specific settings, using the D-Bus <-> KConfigXT bridge
    const KConfigGroup settings = KConfigGroup(mCurrentDefault, QLatin1String("Settings"));

    const QStringList lstSettings = settings.keyList();
    for (const QString &setting : lstSettings) {
        qCDebug(AKONADICORE_LOG) << "Setting up " << setting << " for agent " << instance.identifier();
        const QString methodName = QStringLiteral("set%1").arg(setting);
        const QMetaType::Type argType = argumentType(iface->metaObject(), methodName);
        if (argType == QMetaType::UnknownType) {
            qCCritical(AKONADICORE_LOG) << "Setting " << setting << " not found in agent configuration interface of " << instance.identifier();
            continue;
        }

        QVariant arg;
        if (argType == QMetaType::QString) {
            // Since a string could be a path we always use readPathEntry here,
            // that shouldn't harm any normal string settings
            arg = settings.readPathEntry(setting, QString());
        } else {
            arg = settings.readEntry(setting, QVariant(QMetaType(argType)));
        }

        const QDBusReply<void> reply = iface->call(methodName, arg);
        if (!reply.isValid()) {
            qCCritical(AKONADICORE_LOG) << "Setting " << setting << " failed for agent " << instance.identifier();
        }
    }

    iface->call(QStringLiteral("save"));

    instance.reconfigure();
    instance.synchronize();
    delete iface;

    // remember we set this one up already
    KConfigGroup cfg(mConfig, QLatin1String("ProcessedDefaults"));
    cfg.writeEntry(agentCfg.readEntry("Id", QString()), instance.identifier());
    cfg.sync();

    setupNext();
}

QMetaType::Type Firstrun::argumentType(const QMetaObject *metaObject, const QString &method)
{
    QMetaMethod metaMethod;
    for (int i = 0; i < metaObject->methodCount(); ++i) {
        const QString signature = QString::fromLatin1(metaObject->method(i).methodSignature());
        if (signature.startsWith(method)) {
            metaMethod = metaObject->method(i);
        }
    }

    if (metaMethod.methodSignature().isEmpty()) {
        return QMetaType::UnknownType;
    }

    const QList<QByteArray> argTypes = metaMethod.parameterTypes();
    if (argTypes.count() != 1) {
        return QMetaType::UnknownType;
    }

    return static_cast<QMetaType::Type>(QMetaType::fromName(argTypes.first().constData()).id());
}

#include "moc_firstrun_p.cpp"
