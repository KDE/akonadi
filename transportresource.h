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

#ifndef AKONADI_TRANSPORTRESOURCE_H
#define AKONADI_TRANSPORTRESOURCE_H

#include "akonadi_export.h"

#include <QString>

#include <akonadi/item.h>
#include <akonadi/resourcebase.h>

class KJob;

namespace Akonadi {

/**
  * @short Resource implementing mail transport capability.
  *
  * If your resource supports sending email, inherit from this class instead
  * of ResourceBase.  Then implement the virtual method send().
  *
  * For an example of a transport-enabled resource, see
  * kdepim/akonadi/resources/mailtransport_dummy.
 */

/*
   DESIGN NOTES:

   There seem to be three ways to implement transport-enabled resources:
   1) new class TransportResource (this):
      Pros:
      + Straight-forward API, no need to connect to weird signals, just
        reimplement a virtual method.
      + The TransportAdaptor is taken care of here, and resources need not
        worry about it.
      Cons:
      - If there will ever be SomeOtherNiftyFeatureResource derived from
        ResourceBase, then it will be hard to combine the Transport ability
        (this class) with SomeOtherNiftyFeature.
    2) add the stuff to ResourceBase:
      Pros:
      + No additional class, so further extensions seem possible.
      Cons:
      - The TransportAdaptor either has to be merged with ResourceAdaptor;
        added in ResourceBase; or compiled from xml and added by each
        transport-enabled resource.  This means either useless weight to all
        resources, or added complexity and dependencies for transport-enabled
        resources.
      - Weird API.  Transport-enabled resources will need to connect to a
        transportItem() signal, and there is no guarantee that this is done
        properly (unlike a virtual function).
    3) do it with ObserverV2:
       How?
*/

class AKONADI_EXPORT TransportResource : public ResourceBase
{
  Q_OBJECT

  Q_SIGNALS:
    /**
      Emitted when the resource has finished sending the message.
      @param result The success of the operation.
      @param message An optional textual explanation of the result.
    */
    void transportResult( bool result, const QString &message );

  public Q_SLOTS:
    /**
      Reimplement in your resource, to begin the actual sending operation.
      Connect the transport job's result() signal to emitResult(), or emit
      transportResult() manually when you are done.
      @param message The ID of the message to be sent.
      @see emitResult.
      @see transportResult.
    */
    virtual void send( Akonadi::Item::Id message ) = 0;

  protected:
    /**
      Constructs a new TransportResource.
      @param id The instance id of the resource.
    */
    TransportResource( const QString &id );

  protected Q_SLOTS:
    /**
      Utility function to emit transportResult.  If your sending operation is
      a KJob, you can connect its result() signal to this slot, and it will
      emit transportResult with the proper parameters.
      @param job The transport job.
    */
    void emitResult( KJob *job );

  private:
    // TODO probably need a private class.

};

} // namespace Akonadi

#endif // AKONADI_TRANSPORTRESOURCE_H
