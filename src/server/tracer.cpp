/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#include "tracer.h"

#include <QSettings>
#include <QString>

#include "traceradaptor.h"

#include "akonadiserver_debug.h"
#include "dbustracer.h"
#include "filetracer.h"

#include <private/standarddirs_p.h>

// #define DEFAULT_TRACER QLatin1StringView( "dbus" )
#define DEFAULT_TRACER QStringLiteral("null")

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
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->beginConnection(identifier, msg);
    }
}

void Tracer::endConnection(const QString &identifier, const QString &msg)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->endConnection(identifier, msg);
    }
}

void Tracer::connectionInput(const QString &identifier, const QByteArray &msg)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->connectionInput(identifier, msg);
    }
}

void Akonadi::Server::Tracer::connectionInput(const QString &identifier, qint64 tag, const Protocol::CommandPtr &cmd)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        if (mTracerBackend->connectionFormat() == TracerInterface::Json) {
            QJsonObject json;
            json[QStringLiteral("tag")] = tag;
            Akonadi::Protocol::toJson(cmd.data(), json);

            QJsonDocument doc(json);

            mTracerBackend->connectionInput(identifier, doc.toJson(QJsonDocument::Indented));
        } else {
            mTracerBackend->connectionInput(identifier, QByteArray::number(tag) + ' ' + Protocol::debugString(cmd).toUtf8());
        }
    }
}

void Tracer::connectionOutput(const QString &identifier, const QByteArray &msg)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->connectionOutput(identifier, msg);
    }
}

void Tracer::connectionOutput(const QString &identifier, qint64 tag, const Protocol::CommandPtr &cmd)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        if (mTracerBackend->connectionFormat() == TracerInterface::Json) {
            QJsonObject json;
            json[QStringLiteral("tag")] = tag;
            Protocol::toJson(cmd.data(), json);
            QJsonDocument doc(json);

            mTracerBackend->connectionOutput(identifier, doc.toJson(QJsonDocument::Indented));
        } else {
            mTracerBackend->connectionOutput(identifier, QByteArray::number(tag) + ' ' + Protocol::debugString(cmd).toUtf8());
        }
    }
}

void Tracer::signal(const QString &signalName, const QString &msg)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->signal(signalName, msg);
    }
}

void Tracer::signal(const char *signalName, const QString &msg)
{
    signal(QLatin1StringView(signalName), msg);
}

void Tracer::warning(const QString &componentName, const QString &msg)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->warning(componentName, msg);
    }
}

void Tracer::error(const QString &componentName, const QString &msg)
{
    QMutexLocker locker(&mMutex);
    if (mTracerBackend) {
        mTracerBackend->error(componentName, msg);
    }
}

void Tracer::error(const char *componentName, const QString &msg)
{
    error(QLatin1StringView(componentName), msg);
}

QString Tracer::currentTracer() const
{
    QMutexLocker locker(&mMutex);
    return mSettings->value(QStringLiteral("Debug/Tracer"), DEFAULT_TRACER).toString();
}

void Tracer::activateTracer(const QString &type)
{
    QMutexLocker locker(&mMutex);

    if (type == QLatin1StringView("file")) {
        const QString file = mSettings->value(QStringLiteral("Debug/File"), QStringLiteral("/dev/null")).toString();
        mTracerBackend = std::make_unique<FileTracer>(file);
    } else if (type == QLatin1StringView("dbus")) {
        mTracerBackend = std::make_unique<DBusTracer>();
    } else if (type == QLatin1StringView("null")) {
        mTracerBackend.reset();
    } else {
        qCCritical(AKONADISERVER_LOG) << "Unknown tracer type" << type;
        mTracerBackend.reset();
        return;
    }

    mSettings->setValue(QStringLiteral("Debug/Tracer"), type);
    mSettings->sync();
}

#include "moc_tracer.cpp"
