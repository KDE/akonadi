/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "dbustracer.h"
#include "tracernotificationadaptor.h"

using namespace Akonadi;

DBusTracer::DBusTracer()
  : QObject( 0 )
{
  new TracerNotificationAdaptor( this );

  QDBus::sessionBus().registerObject( "/tracing", this, QDBusConnection::ExportAdaptors );
}

DBusTracer::~DBusTracer()
{
}

void DBusTracer::beginConnection( const QString &identifier, const QString &msg )
{
  emit connectionStarted( identifier, msg );
}

void DBusTracer::endConnection( const QString &identifier, const QString &msg )
{
  emit connectionEnded( identifier, msg );
}

void DBusTracer::connectionInput( const QString &identifier, const QString &msg )
{
  emit connectionDataInput( identifier, msg );
}

void DBusTracer::connectionOutput( const QString &identifier, const QString &msg )
{
  emit connectionDataOutput( identifier, msg );
}

void DBusTracer::signal( const QString &signalName, const QString &msg )
{
  emit signalEmitted( signalName, msg );
}

void DBusTracer::warning( const QString &componentName, const QString &msg )
{
  emit warningEmitted( componentName, msg );
}

void DBusTracer::error( const QString &componentName, const QString &msg )
{
  emit errorEmitted( componentName, msg );
}

#include "dbustracer.moc"
