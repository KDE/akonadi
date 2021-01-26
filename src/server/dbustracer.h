/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_DBUSTRACER_H
#define AKONADI_DBUSTRACER_H

#include <QObject>

#include "tracerinterface.h"

namespace Akonadi
{
namespace Server
{
/**
 * A tracer which forwards all tracing information as dbus signals.
 */
class DBusTracer : public QObject, public TracerInterface
{
    Q_OBJECT

public:
    DBusTracer();
    ~DBusTracer() override;

    void beginConnection(const QString &identifier, const QString &msg) override;
    void endConnection(const QString &identifier, const QString &msg) override;
    void connectionInput(const QString &identifier, const QByteArray &msg) override;
    void connectionOutput(const QString &identifier, const QByteArray &msg) override;
    void signal(const QString &signalName, const QString &msg) override;
    void warning(const QString &componentName, const QString &msg) override;
    void error(const QString &componentName, const QString &msg) override;

    TracerInterface::ConnectionFormat connectionFormat() const override
    {
        return TracerInterface::Json;
    }

Q_SIGNALS:
    void connectionStarted(const QString &identifier, const QString &msg);
    void connectionEnded(const QString &identifier, const QString &msg);
    void connectionDataInput(const QString &identifier, const QString &msg);
    void connectionDataOutput(const QString &identifier, const QString &msg);
    void signalEmitted(const QString &signalName, const QString &msg);
    void warningEmitted(const QString &componentName, const QString &msg);
    void errorEmitted(const QString &componentName, const QString &msg);
};

} // namespace Server
} // namespace Akonadi

#endif
