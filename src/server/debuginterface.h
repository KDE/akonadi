/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_DEBUGINTERFACE_H
#define AKONADI_DEBUGINTERFACE_H

#include <QObject>

namespace Akonadi
{
namespace Server
{

class Tracer;

/**
 * Interface to configure and query debugging options.
 */
class DebugInterface : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.DebugInterface")

public:
    explicit DebugInterface(Tracer &tracer);

public Q_SLOTS:
    Q_SCRIPTABLE QString tracer() const;
    Q_SCRIPTABLE void setTracer(const QString &tracer);

private:
    Tracer &m_tracer;
};

} // namespace Server
} // namespace Akonadi

#endif
