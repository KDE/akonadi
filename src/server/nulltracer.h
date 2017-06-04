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

#ifndef AKONADI_NULLTRACER_H
#define AKONADI_NULLTRACER_H

#include "tracerinterface.h"

namespace Akonadi
{
namespace Server
{

/**
 * A tracer which forwards all tracing information to /dev/null ;)
 */
class NullTracer : public TracerInterface
{
public:
    virtual ~NullTracer()
    {
    }

    void beginConnection(const QString &identifier, const QString &msg) override {
        Q_UNUSED(identifier);
        Q_UNUSED(msg);
    }

    void endConnection(const QString &identifier, const QString &msg) override {
        Q_UNUSED(identifier);
        Q_UNUSED(msg);
    }

    void connectionInput(const QString &identifier, const QByteArray &msg) override {
        Q_UNUSED(identifier);
        Q_UNUSED(msg);
    }

    void connectionOutput(const QString &identifier, const QByteArray &msg) override {
        Q_UNUSED(identifier);
        Q_UNUSED(msg);
    }

    void signal(const QString &signalName, const QString &msg) override {
        Q_UNUSED(signalName);
        Q_UNUSED(msg);
    }

    void warning(const QString &componentName, const QString &msg) override {
        Q_UNUSED(componentName);
        Q_UNUSED(msg);
    }

    void error(const QString &componentName, const QString &msg) override {
        Q_UNUSED(componentName);
        Q_UNUSED(msg);
    }
};

} // namespace Server
} // namespace Akonadi

#endif
