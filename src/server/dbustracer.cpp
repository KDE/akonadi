/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
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

DBusTracer::~DBusTracer() = default;

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
