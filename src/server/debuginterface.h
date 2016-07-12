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

#ifndef AKONADI_DEBUGINTERFACE_H
#define AKONADI_DEBUGINTERFACE_H

#include <QObject>

namespace Akonadi {
namespace Server {

/**
 * Interface to configure and query debugging options.
 */
class DebugInterface : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.DebugInterface")

public:
    explicit DebugInterface(QObject *parent = Q_NULLPTR);

public Q_SLOTS:
    Q_SCRIPTABLE QString tracer() const;
    Q_SCRIPTABLE void setTracer(const QString &tracer);

};

} // namespace Server
} // namespace Akonadi

#endif
