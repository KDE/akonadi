/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_TRANSPORTRESOURCEBASE_H
#define AKONADI_TRANSPORTRESOURCEBASE_H

#include "akonadi_export.h"

#include <QtCore/QString>

#include <akonadi/item.h>

class KJob;

namespace Akonadi {

class TransportResourceBasePrivate;

/**
  * @short Resource implementing mail transport capability.
  *
  * To implement a transport-enabled resource, inherit from both
  * ResourceBase and TransportResourceBase, implement the virtual method
  * sendItem(), and call emitTransportResult() when finished sending.
  * The resource must also have the "MailTransport" capability flag.
  *
  * For an example of a transport-enabled resource, see
  * kdepim/akonadi/resources/mailtransport_dummy.
  *
  * @author Constantin Berzan <exit3219@gmail.com>
  * @since 4.4
 */
class AKONADI_EXPORT TransportResourceBase
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
    enum TransportResult
    {
      TransportSucceeded, ///< The transport process succeeded.
      TransportFailed     ///< The transport process failed.
    };

    /**
     * This method is called when the given @p item shall be send.
     * When the sending is done or an error occurred during
     * sending, call itemSent() with the according result flag.
     *
     * @param item The message item to be send.
     * @see itemSent().
     */
    virtual void sendItem( const Akonadi::Item &item ) = 0;

    /**
     * This method marks the sending of the passed @p item
     * as finished.
     *
     * @param item The item that was sent.
     * @param result The result that indicates whether the sending
     *               was successfully or not.
     * @param message An optional textual explanation of the result.
     * @see Transport.
     */
    void itemSent( const Akonadi::Item &item, TransportResult result,
                   const QString &message = QString() );

  private:
    //@cond PRIVATE
    TransportResourceBasePrivate *const d;
    //@endcond
};

}

#endif
