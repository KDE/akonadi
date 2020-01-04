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
#include "filetracer.h"

#include <QFile>
#include <QTime>

using namespace Akonadi::Server;

FileTracer::FileTracer(const QString &fileName)
    : m_file(fileName)
{
    m_file.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
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
    QString output = QStringLiteral("%1: %2: %3\r\n").arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")), id, msg.left(msg.indexOf(QLatin1Char('\n'))));
    m_file.write(output.toUtf8());
}
