/*
    This file is part of akonadiresources.

    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentfactory.h"
#include "servermanager.h"
#include "servermanager_p.h"

#include <KGlobal>
#include <KLocalizedString>

#include <QThread>
#include <QThreadStorage>

QThreadStorage<KComponentData *> s_agentComponentDatas;

using namespace Akonadi;

class Akonadi::AgentFactoryBasePrivate
{
public:
    QString catalogName;
};

AgentFactoryBase::AgentFactoryBase(const char *catalogName, QObject *parent)
    : QObject(parent)
    , d(new AgentFactoryBasePrivate)
{
    d->catalogName = QString::fromLatin1(catalogName);
    if (!KGlobal::hasMainComponent()) {
        new KComponentData("AkonadiAgentServer", "libakonadi", KComponentData::RegisterAsMainComponent);
    }
    KLocalizedString::setApplicationDomain(catalogName);

    Internal::setClientType(Internal::Agent);
    ServerManager::self(); // make sure it's created in the main thread
}

AgentFactoryBase::~AgentFactoryBase() = default;

void AgentFactoryBase::createComponentData(const QString &identifier) const
{
    Q_ASSERT(!s_agentComponentDatas.hasLocalData());

    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        s_agentComponentDatas.setLocalData(
            new KComponentData(ServerManager::addNamespace(identifier).toLatin1(), d->catalogName.toLatin1(), KComponentData::SkipMainComponentRegistration));
    } else {
        s_agentComponentDatas.setLocalData(new KComponentData(ServerManager::addNamespace(identifier).toLatin1(), d->catalogName.toLatin1()));
    }
}

#include "moc_agentfactory.cpp"
