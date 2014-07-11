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

#include "agentfactory.h"
#include "servermanager.h"
#include "servermanager_p.h"

#include <klocale.h>
#include <klocalizedstring.h>
#include <kglobal.h>

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>

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
    KLocalizedString::setApplicationDomain( catalogName ); 

    Internal::setClientType(Internal::Agent);
    ServerManager::self(); // make sure it's created in the main thread
}

AgentFactoryBase::~AgentFactoryBase()
{
    delete d;
}

void AgentFactoryBase::createComponentData(const QString &identifier) const
{
    Q_ASSERT(!s_agentComponentDatas.hasLocalData());

    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        s_agentComponentDatas.setLocalData(new KComponentData(ServerManager::addNamespace(identifier).toLatin1(), d->catalogName.toLatin1(),
                                                              KComponentData::SkipMainComponentRegistration));
    } else {
        s_agentComponentDatas.setLocalData(new KComponentData(ServerManager::addNamespace(identifier).toLatin1(), d->catalogName.toLatin1()));
    }
}
