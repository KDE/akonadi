/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#include "filetracer.h"

#include <QTime>

using namespace Akonadi::Server;

FileTracer::FileTracer(const QString &fileName)
    : m_file(fileName)
{
    std::ignore = m_file.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
}

FileTracer::~FileTracer() = default;

void FileTracer::beginConnection(const QString &identifier, const QString &msg)
{
    output(identifier, QStringLiteral("begin_connection: %1").arg(msg));
}

void FileTracer::endConnection(const QString &identifier, const QString &msg)
{
    output(identifier, QStringLiteral("end_connection: %1").arg(msg));
}

void FileTracer::connectionInput(const QString &identifier, const QByteArray &msg)
{
    output(identifier, QStringLiteral("input: %1").arg(QString::fromUtf8(msg)));
}

void FileTracer::connectionOutput(const QString &identifier, const QByteArray &msg)
{
    output(identifier, QStringLiteral("output: %1").arg(QString::fromUtf8(msg)));
}

void FileTracer::signal(const QString &signalName, const QString &msg)
{
    output(QStringLiteral("signal"), QStringLiteral("<%1> %2").arg(signalName, msg));
}

void FileTracer::warning(const QString &componentName, const QString &msg)
{
    output(QStringLiteral("warning"), QStringLiteral("<%1> %2").arg(componentName, msg));
}

void FileTracer::error(const QString &componentName, const QString &msg)
{
    output(QStringLiteral("error"), QStringLiteral("<%1> %2").arg(componentName, msg));
}

void FileTracer::output(const QString &id, const QString &msg)
{
    QString output = QStringLiteral("%1: %2: %3\r\n").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")), id, msg.left(msg.indexOf(u'\n')));
    m_file.write(output.toUtf8());
}
