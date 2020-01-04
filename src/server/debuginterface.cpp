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

#include "debuginterface.h"
#include "debuginterfaceadaptor.h"
#include "tracer.h"
#include "akonadi.h"

#include <QDBusConnection>

using namespace Akonadi::Server;

DebugInterface::DebugInterface(AkonadiServer &akonadi)
    : QObject()
    , m_akonadi(akonadi)
{
    new DebugInterfaceAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/debug"),
            this, QDBusConnection::ExportAdaptors);
}

QString DebugInterface::tracer() const
{
    return m_akonadi.tracer().currentTracer();
}

void DebugInterface::setTracer(const QString &tracer)
{
    m_akonadi.tracer().activateTracer(tracer);
}
