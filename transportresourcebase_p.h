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

#ifndef AKONADI_TRANSPORTRESOURCEBASE_P_H
#define AKONADI_TRANSPORTRESOURCEBASE_P_H

#include "transportresourcebase.h"

#include <QtCore/QObject>

class TransportAdaptor;

namespace Akonadi {

class TransportResourceBase;

/**
  @internal
  This class hosts the D-Bus adaptor for TransportResourceBase.
*/
class TransportResourceBasePrivate : public QObject
{
  Q_OBJECT
  public:
    TransportResourceBasePrivate( TransportResourceBase *qq );

  signals:
    /**
     * Emitted when an item has been sent.
     * @param item The id of the item that was sent.
     * @param success The success of the sending operation.
     * @param message An optional textual explanation of the result.
     * @since 4.4
     */
    void transportResult( qlonglong item, bool success, const QString &message ); // D-Bus signal

  private:
    friend class TransportResourceBase;
    friend class ::TransportAdaptor;

    void send( Akonadi::Item::Id message ); // D-Bus call

    TransportResourceBase *const q;
};

} // namespace Akonadi

#endif // AKONADI_TRANSPORTRESOURCEBASE_P_H
