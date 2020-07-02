/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADI_TRACERINTERFACE_H
#define AKONADI_TRACERINTERFACE_H

#include <QtGlobal>

class QByteArray;
class QString;

namespace Akonadi
{
namespace Server
{

/**
 * This interface can be reimplemented to deliver tracing information
 * of the akonadi server to the outside.
 *
 * Possible implementations:
 *   - log file
 *   - dbus signals
 *   - live gui
 */
class TracerInterface
{
public:
    enum ConnectionFormat {
        DebugString,
        Json
    };

    virtual ~TracerInterface() = default;

    /**
     * This method is called whenever a new data (imap) connection to the akonadi server
     * is established.
     *
     * @param identifier The unique identifier for this connection. All input and output
     *                   messages for this connection will have the same identifier.
     *
     * @param msg A message specific string.
     */
    virtual void beginConnection(const QString &identifier, const QString &msg) = 0;

    /**
     * This method is called whenever a data (imap) connection to akonadi server is
     * closed.
     *
     * @param identifier The unique identifier of this connection.
     * @param msg A message specific string.
     */
    virtual void endConnection(const QString &identifier, const QString &msg) = 0;

    /**
     * This method is called whenever the akonadi server retrieves some data from the
     * outside.
     *
     * @param identifier The unique identifier of the connection on which the data
     *                   is retrieved.
     * @param msg A message specific string.
     */
    virtual void connectionInput(const QString &identifier, const QByteArray &msg) = 0;

    /**
     * This method is called whenever the akonadi server sends some data out to a client.
     *
     * @param identifier The unique identifier of the connection on which the
     *                   data is send.
     * @param msg A message specific string.
     */
    virtual void connectionOutput(const QString &identifier, const QByteArray &msg) = 0;

    /**
     * This method is called whenever a dbus signal is emitted on the bus.
     *
     * @param signalName The name of the signal being sent.
     * @param msg A message specific string.
     */
    virtual void signal(const QString &signalName, const QString &msg) = 0;

    /**
     * This method is called whenever a component wants to output a warning.
     */
    virtual void warning(const QString &componentName, const QString &msg) = 0;

    /**
     * This method is called whenever a component wants to output an error.
     */
    virtual void error(const QString &componentName, const QString &msg) = 0;

    virtual ConnectionFormat connectionFormat() const {return DebugString;}

protected:
    explicit TracerInterface() = default;

private:
    Q_DISABLE_COPY_MOVE(TracerInterface)
};

} // namespace Server
} // namespace Akonadi

#endif
