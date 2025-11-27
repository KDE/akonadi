/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QObject>

#include "private/protocol_p.h"
#include "tracerinterface.h"

#include <memory>

class QSettings;

namespace Akonadi
{
namespace Protocol
{
class Command;
using CommandPtr = QSharedPointer<Command>;
}

namespace Server
{
/**
 * The global tracer instance where all akonadi components can
 * send their tracing information to.
 *
 * The tracer will forward these information to the configured backends.
 */
class Tracer : public QObject, public TracerInterface
{
    Q_OBJECT

public:
    explicit Tracer();

    /**
     * Destroys the global tracer instance.
     */
    ~Tracer() override;

    template<typename T>
    typename std::enable_if<std::is_base_of<Protocol::Command, T>::value>::type connectionOutput(const QString &identifier, qint64 tag, const T &cmd)
    {
        QByteArray msg;
        if (mTracerBackend->connectionFormat() == TracerInterface::Json) {
            QJsonObject json;
            json[QStringLiteral("tag")] = tag;
            cmd.toJson(json);
            QJsonDocument doc(json);

            msg = doc.toJson(QJsonDocument::Indented);
        } else {
            msg = QByteArray::number(tag) + ' ' + Protocol::debugString(cmd).toUtf8();
        }
        connectionOutput(identifier, msg);
    }

    /**
     * Returns the currently activated tracer type.
     */
    QString currentTracer() const;

public Q_SLOTS:
    /**
     * This method is called whenever a new data (imap) connection to the akonadi server
     * is established.
     *
     * @param identifier The unique identifier for this connection. All input and output
     *                   messages for this connection will have the same identifier.
     *
     * @param msg A message specific string.
     */
    void beginConnection(const QString &identifier, const QString &msg) override;

    /**
     * This method is called whenever a data (imap) connection to akonadi server is
     * closed.
     *
     * @param identifier The unique identifier of this connection.
     * @param msg A message specific string.
     */
    void endConnection(const QString &identifier, const QString &msg) override;

    /**
     * This method is called whenever the akonadi server retrieves some data from the
     * outside.
     *
     * @param identifier The unique identifier of the connection on which the data
     *                   is retrieved.
     * @param msg A message specific string.
     */
    void connectionInput(const QString &identifier, const QByteArray &msg) override;

    void connectionInput(const QString &identifier, qint64 tag, const Akonadi::Protocol::CommandPtr &cmd);

    /**
     * This method is called whenever the akonadi server sends some data out to a client.
     *
     * @param identifier The unique identifier of the connection on which the
     *                   data is send.
     * @param msg A message specific string.
     */
    void connectionOutput(const QString &identifier, const QByteArray &msg) override;

    void connectionOutput(const QString &identifier, qint64 tag, const Akonadi::Protocol::CommandPtr &cmd);

    /**
     * This method is called whenever a dbus signal is emitted on the bus.
     *
     * @param signalName The name of the signal being sent.
     * @param msg A message specific string.
     */
    void signal(const QString &signalName, const QString &msg) override;

    /**
      Convenience method with internal toLatin1 cast to compile with QT_NO_CAST_FROM_ASCII.
    */
    void signal(const char *signalName, const QString &msg);

    /**
     * This method is called whenever a component wants to output a warning.
     */
    void warning(const QString &componentName, const QString &msg) override;

    /**
     * This method is called whenever a component wants to output an error.
     */
    void error(const QString &componentName, const QString &msg) override;

    /**
     * Convenience method for QT_NO_CAST_FROM_ASCII usage.
     */
    void error(const char *componentName, const QString &msg);

    /**
     * Activates the given tracer type.
     */
    void activateTracer(const QString &type);

private:
    mutable QMutex mMutex;
    std::unique_ptr<TracerInterface> mTracerBackend;
    std::unique_ptr<QSettings> mSettings;
};

} // namespace Server
} // namespace Akonadi
