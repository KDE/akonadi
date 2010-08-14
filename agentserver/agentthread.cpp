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

using namespace Akonadi;

AgentThread::AgentThread(const QString& identifier, const QString& fileName, QObject* parent):
  QThread(parent),
  m_identifier(identifier),
  m_fileName(fileName),
  m_instance(0)
{
}

void AgentThread::run()
{
  qDebug() << Q_FUNC_INFO << m_fileName;
  QPluginLoader loader( m_fileName );
  if ( !loader.load() ) {
    akError() << Q_FUNC_INFO << "Failed to load agent: " << loader.errorString();
    return;
  }

  m_instance = loader.instance();
  qDebug() << Q_FUNC_INFO << "Loading agent instance succeeded: " << m_instance;
  exec();
}

#include "agentthread.moc"
