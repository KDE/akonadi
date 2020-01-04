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
#include "tracer.h"

#include <QSettings>
#include <QString>

#include "traceradaptor.h"

#include "dbustracer.h"
#include "filetracer.h"
#include "nulltracer.h"

#include <private/standarddirs_p.h>

// #define DEFAULT_TRACER QLatin1String( "dbus" )
#define DEFAULT_TRACER QStringLiteral( "null" )

using namespace Akonadi;
using namespace Akonadi::Server;

Tracer::Tracer()
    : mSettings(std::make_unique<QSettings>(Akonadi::StandardDirs::serverConfigFile(), QSettings::IniFormat))
{
    activateTracer(currentTracer());

    new TracerAdaptor(this);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/tracing"), this, QDBusConnection::ExportAdaptors);
}

Tracer::~Tracer() = default;

void Tracer::beginConnection(const QString &identifier, const QString &msg)
{
    mMutex.lock();
    mTracerBackend->beginConnection(identifier, msg);
    mMutex.unlock();
}

void Tracer::endConnection(const QString &identifier, const QString &msg)
{
    mMutex.lock();
    mTracerBackend->endConnection(identifier, msg);
    mMutex.unlock();
}

void Tracer::connectionInput(const QString &identifier, const QByteArray &msg)
{
    mMutex.lock();
    mTracerBackend->connectionInput(identifier, msg);
    mMutex.unlock();
}

void Akonadi::Server::Tracer::connectionInput(const QString& identifier, qint64 tag, const Protocol::CommandPtr &cmd)
{
    QByteArray msg;
    if (mTracerBackend->connectionFormat() == TracerInterface::Json) {
        QJsonObject json;
        json[QStringLiteral("tag")] = tag;
        Akonadi::Protocol::toJson(cmd.data(), json);

        QJsonDocument doc(json);

        msg = doc.toJson(QJsonDocument::Indented);
    } else {
        msg = QByteArray::number(tag) + ' ' + Protocol::debugString(cmd).toUtf8();
    }
    connectionInput(identifier, msg);
}

void Tracer::connectionOutput(const QString &identifier, const QByteArray &msg)
{
    mMutex.lock();
    mTracerBackend->connectionOutput(identifier, msg);
    mMutex.unlock();
}

void Tracer::connectionOutput(const QString &identifier, qint64 tag, const Protocol::CommandPtr &cmd)
{
    QByteArray msg;
    if (mTracerBackend->connectionFormat() == TracerInterface::Json) {
        QJsonObject json;
        json[QStringLiteral("tag")] = tag;
        Protocol::toJson(cmd.data(), json);
        QJsonDocument doc(json);

        msg = doc.toJson(QJsonDocument::Indented);
    } else {
        msg = QByteArray::number(tag) + ' ' + Protocol::debugString(cmd).toUtf8();
    }
    connectionOutput(identifier, msg);
}


void Tracer::signal(const QString &signalName, const QString &msg)
{
    mMutex.lock();
    mTracerBackend->signal(signalName, msg);
    mMutex.unlock();
}

void Tracer::signal(const char *signalName, const QString &msg)
{
    signal(QLatin1String(signalName), msg);
}

void Tracer::warning(const QString &componentName, const QString &msg)
{
    mMutex.lock();
    mTracerBackend->warning(componentName, msg);
    mMutex.unlock();
}

void Tracer::error(const QString &componentName, const QString &msg)
{
    mMutex.lock();
    mTracerBackend->error(componentName, msg);
    mMutex.unlock();
}

void Tracer::error(const char *componentName, const QString &msg)
{
    error(QLatin1String(componentName), msg);
}

QString Tracer::currentTracer() const
{
    QMutexLocker locker(&mMutex);
    return mSettings->value(QStringLiteral("Debug/Tracer"), DEFAULT_TRACER).toString();
}

void Tracer::activateTracer(const QString &type)
{
    QMutexLocker locker(&mMutex);

    mSettings->setValue(QStringLiteral("Debug/Tracer"), type);
    mSettings->sync();

    if (type == QLatin1String("file")) {
        const QString file = mSettings->value(QStringLiteral("Debug/File"), QStringLiteral("/dev/null")).toString();
        mTracerBackend = std::make_unique<FileTracer>(file);
    } else if (type == QLatin1String("null")) {
        mTracerBackend = std::make_unique<NullTracer>();
    } else {
        mTracerBackend = std::make_unique<DBusTracer>();
    }
    Q_ASSERT(mTracerBackend);
}
