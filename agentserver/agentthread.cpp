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

#include "agentthread.h"

#include <QDebug>
#include <QtCore/qpluginloader.h>
#include <shared/akdebug.h>
#include <qmetaobject.h>

using namespace Akonadi;

AgentThread::AgentThread(const QString& identifier, QObject *factory, QObject* parent):
  QThread(parent),
  m_identifier(identifier),
  m_factory( factory ),
  m_instance(0)
{
}

void AgentThread::run()
{
  qDebug() << Q_FUNC_INFO << "Loading agent instance succeeded: " << m_factory;
  for ( int i = 0; i < m_factory->metaObject()->methodCount(); ++i )
    qDebug() << m_factory->metaObject()->method( i ).signature();
  if ( QMetaObject::invokeMethod( m_factory, "createInstance", Qt::DirectConnection, Q_RETURN_ARG(QObject*, m_instance), Q_ARG(QString, m_identifier) ) )
    qDebug() << Q_FUNC_INFO << "agent instance created: " << m_instance;
  else
    qDebug() << Q_FUNC_INFO << "agent instance creation failed";
  exec();
}

#include "agentthread.moc"
