/*
    Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>

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

#ifndef AKONADI_FREEBUSYPROVIDERBASE_H
#define AKONADI_FREEBUSYPROVIDERBASE_H

#include "akonadi-calendar_export.h"

#include <QtCore/QString>

class KDateTime;

namespace Akonadi
{

class FreeBusyProviderBasePrivate;

/**
 * @short Base class for resources providing free-busy information
 *
 * This class must be inherited by resources that are able to provide
 * free-busy information for a given contact on request. A resource
 * will thus inherit from ResourceBase and FreeBusyProvider.
 *
 * Resources that provide FB info must declare it by adding
 * 'FreeBusyProvider' in the X-Akonadi-Capabilities field of their
 * .desktop file:
 \code
 X-Akonadi-Capabilities=Resource,FreeBusyProvider
 \endcode
 *
 * Resource inheriting from this class must implement lastCacheUpdate(),
 * canHandleFreeBusy() and retrieveFreeBusy().
 *
 * @since 4.7
 */

class AKONADI_CALENDAR_EXPORT FreeBusyProviderBase
{
  public:
    /**
     * Creates a new FreeBusyProvider
     */
    FreeBusyProviderBase();

    /**
     * Destroys a FreeBusyProvider
     */
    virtual ~FreeBusyProviderBase();

    /**
     * Returns the last time the free-busy information was
     * fetched from the server. This can be used for example
     * to issue a warning to the user that this information
     * may not be accurate and must be refreshed; pretty useful
     * when the resource was offline for too long.
     *
     * @return The date and time the cache was last updated.
     */
    virtual KDateTime lastCacheUpdate() const = 0;

    /**
     * This method is called to find out is the resource
     * handle free-busy information for the contact with
     * email address @p email.
     *
     * The caller will not wait for the result. Once the
     * decision is known, the resource must call
     * handlesFreeBusy().
     *
     * @param email The email address of the contact we want
     *              the free-busy info. This is a simple email
     *              address, in the form foo@bar.com (no display
     *              name or quoting).
     * @see handlesFreeBusy()
     *
     */
    virtual void canHandleFreeBusy( const QString &email ) const = 0;

    /**
     * Derivate classes must call this method once they know
     * if they handle free-busy information for the contact
     * with email address @p email.
     *
     * @param email The email address of the contact we give the
     *              response for. This is a simple email
     *              address, in the form foo@bar.com (no display
     *              name or quoting).
     * @param handles Whether this resource handles free-busy (true)
     *              or not (false).
     */
    void handlesFreeBusy( const QString &email, bool handles ) const;

    /**
     * This method is called when the resource must do the real
     * work and fetch the free-busy information for the contact
     * with email address @p email.
     *
     * As with canHandleFreeBusy() the caller will not wait for
     * the result and the resource must call freeBusyRetrieved()
     * once done.
     *
     * @param email The email address of the contact we want
     *              the free-busy info. This is a simple email
     *              address, in the form foo@bar.com (no display
     *              name or quoting).
     * @param start The start of the period the free-busy request covers
     * @param end The end of the free-busy period
     * @see freeBusyRetrieved()
     */
    virtual void retrieveFreeBusy( const QString &email, const KDateTime &start, const KDateTime &end ) = 0;

    /**
     * Derivate classes must call this method to notify the requestor
     * that the result is here.
     *
     * The @p freeBusy is expected to be an iTIP request containing
     * the free-busy data. The simplest way to generate this is
     * to use KCalCore::ICalFormat::createScheduleMessage()
     * with the method KCalCore::iTIPRequest.
     *
     * @param email The email address of the contact we give the
     *              response for. This is a simple email
     *              address, in the form foo@bar.com (no display
     *              name or quoting).
     * @param freeBusy The free-busy data.
     * @param success Whether the retrieval was successful or not.
     * @param errorText An optional error message that can be displayed back
     *                  to the user.
     */
    void freeBusyRetrieved( const QString &email, const QString &freeBusy, bool success, const QString &errorText = QString() );

  private:
    //@cond PRIVATE
    FreeBusyProviderBasePrivate *const d;
    //@endcond
};

}

#endif // AKONADI_FREEBUSYPROVIDERBASE_H
