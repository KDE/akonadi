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

using namespace Akonadi::Server;

DBusTracer::DBusTracer()
    : QObject(nullptr)
{
    new TracerNotificationAdaptor(this);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/tracing/notifications"), this, QDBusConnection::ExportAdaptors);
}

DBusTracer::~DBusTracer()
{
}

void DBusTracer::beginConnection(const QString &identifier, const QString &msg)
{
    Q_EMIT connectionStarted(identifier, msg);
}

void DBusTracer::endConnection(const QString &identifier, const QString &msg)
{
    Q_EMIT connectionEnded(identifier, msg);
}

void DBusTracer::connectionInput(const QString &identifier, const QByteArray &msg)
{
    Q_EMIT connectionDataInput(identifier, QString::fromUtf8(msg));
}

void DBusTracer::connectionOutput(const QString &identifier, const QByteArray &msg)
{
    Q_EMIT connectionDataOutput(identifier, QString::fromUtf8(msg));
}

void DBusTracer::signal(const QString &signalName, const QString &msg)
{
    Q_EMIT signalEmitted(signalName, msg);
}

void DBusTracer::warning(const QString &componentName, const QString &msg)
{
    Q_EMIT warningEmitted(componentName, msg);
}

void DBusTracer::error(const QString &componentName, const QString &msg)
{
    Q_EMIT errorEmitted(componentName, msg);
}
