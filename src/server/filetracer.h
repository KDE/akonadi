/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include "tracerinterface.h"

#include <QFile>

namespace Akonadi
{
namespace Server
{
/**
 * A tracer which forwards all tracing information to a
 * log file.
 */
class FileTracer : public TracerInterface
{
public:
    explicit FileTracer(const QString &fileName);
    ~FileTracer() override;

    void beginConnection(const QString &identifier, const QString &msg) override;
    void endConnection(const QString &identifier, const QString &msg) override;
    void connectionInput(const QString &identifier, const QByteArray &msg) override;
    void connectionOutput(const QString &identifier, const QByteArray &msg) override;
    void signal(const QString &signalName, const QString &msg) override;
    void warning(const QString &componentName, const QString &msg) override;
    void error(const QString &componentName, const QString &msg) override;

private:
    void output(const QString &id, const QString &msg);

    QFile m_file;
};

} // namespace Server
} // namespace Akonadi
