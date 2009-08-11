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

#include <QString>

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
    TransportResourceBase();
    virtual ~TransportResourceBase();

    /**
      Reimplement in your resource, to begin the actual sending operation.
      Call emitTransportResult() when finished.
      @param message The ID of the message to be sent.
      @see emitTransportResult.
    */
    virtual void sendItem( Akonadi::Item::Id message ) = 0;

    // TODO add void emitSendProgress( int percent );

    /**
     * Emits the transportResult() DBus signal with the given arguments.
     * @param item The id of the item that was sent.
     * @param success True if the sending operation succeeded.
     * @param message An optional textual explanation of the result.
     * @see Transport.
     */
    void emitTransportResult( Akonadi::Item::Id &item, bool success,
                              const QString &message = QString() );

  private:
    TransportResourceBasePrivate *const d;
};

} // namespace Akonadi

#endif // AKONADI_TRANSPORTRESOURCEBASE_H
