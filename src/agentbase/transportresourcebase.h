/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_TRANSPORTRESOURCEBASE_H
#define AKONADI_TRANSPORTRESOURCEBASE_H

#include "akonadiagentbase_export.h"
#include "item.h"

#include <QString>

namespace Akonadi
{
class TransportResourceBasePrivate;

/**
  * @short Resource implementing mail transport capability.
  *
  * This class allows a resource to provide mail transport (i.e. sending
  * mail). A resource than can provide mail transport inherits from both
  * ResourceBase and TransportResourceBase, implements the virtual method
  * sendItem(), and calls itemSent() when finished sending.
  *
  * The resource must also have the "MailTransport" capability flag. For example
  * the desktop file may contain:
    \code
    X-Akonadi-Capabilities=Resource,MailTransport
    \endcode
  *
  * For an example of a transport-enabled resource, see
  * kdepim/runtime/resources/mailtransport_dummy
  *
  * @author Constantin Berzan <exit3219@gmail.com>
  * @since 4.4
 */
class AKONADIAGENTBASE_EXPORT TransportResourceBase
{
public:
    /**
     * Creates a new transport resource base.
     */
    TransportResourceBase();

    /**
     * Destroys the transport resource base.
     */
    virtual ~TransportResourceBase();

    /**
     * Describes the result of the transport process.
     */
    enum TransportResult {
        TransportSucceeded, ///< The transport process succeeded.
        TransportFailed ///< The transport process failed.
    };

    /**
     * This method is called when the given @p item shall be send.
     * When the sending is done or an error occurred during
     * sending, call itemSent() with the appropriate result flag.
     *
     * @param item The message item to be send.
     * @see itemSent().
     */
    virtual void sendItem(const Akonadi::Item &item) = 0;

    /**
     * This method marks the sending of the passed @p item
     * as finished.
     *
     * @param item The item that was sent.
     * @param result The result that indicates whether the sending
     *               was successful or not.
     * @param message An optional text explanation of the result.
     * @see Transport.
     */
    void itemSent(const Akonadi::Item &item, TransportResult result, const QString &message = QString());

private:
    //@cond PRIVATE
    TransportResourceBasePrivate *const d;
    //@endcond
};

}

#endif
