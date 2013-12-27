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

#ifndef AKONADI_FREEBUSYPROVIDERBASEPRIVATE_H
#define AKONADI_FREEBUSYPROVIDERBASEPRIVATE_H

#include "freebusyproviderbase.h"

#include <QtCore/QObject>

class Akonadi__FreeBusyProviderAdaptor;

namespace Akonadi
{

/**
 * @internal
 * This class implements the D-Bus interface of FreeBusyProviderBase
 *
 * @since 4.7
 */
class FreeBusyProviderBasePrivate : public QObject
{
    Q_OBJECT

public:
    explicit FreeBusyProviderBasePrivate(FreeBusyProviderBase *qq);

Q_SIGNALS:
    /**
      * This signal gets emitted when the resource answered
      * the free-busy handling request.
      *
      * @param email The email address of the contact the resource
      *              answered for.
      * @param handles Whether the resource handles free-busy information
      *              (true) or not (false).
      */
    void handlesFreeBusy(const QString &email, bool handles);

    /**
      * This signal gets emitted when the resource answered the
      * free-busy retrieval request.
      *
      * @param email The email address of the contact the resource
      *              answers for.
      * @param freeBusy The free-busy data in iCal format.
      * @param success Whether the retrieval was successful or not.
      * @param errorText A human friendly error message in case something
      *                  went wrong.
      */
    void freeBusyRetrieved(const QString &email, const QString &freeBusy, bool success, const QString &errorText);

private:
    friend class FreeBusyProviderBase;
    friend class ::Akonadi__FreeBusyProviderAdaptor;

    // D-Bus calls
    QString lastCacheUpdate();
    void canHandleFreeBusy(const QString &email);
    void retrieveFreeBusy(const QString &email, const QString &start, const QString &end);

    FreeBusyProviderBase *const q;
};

}

#endif // AKONADI_FREEBUSYPROVIDERBASEPRIVATE_H
