/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "debuginterface.h"
#include "debuginterfaceadaptor.h"
#include "tracer.h"

#include <QDBusConnection>

using namespace Akonadi::Server;

DebugInterface::DebugInterface(Tracer &tracer)
    : m_tracer(tracer)
{
    new DebugInterfaceAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/debug"), this, QDBusConnection::ExportAdaptors);
}

QString DebugInterface::tracer() const
{
    return m_tracer.currentTracer();
}

void DebugInterface::setTracer(const QString &tracer)
{
    m_tracer.activateTracer(tracer);
}

#include "moc_debuginterface.cpp"
