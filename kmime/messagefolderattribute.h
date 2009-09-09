/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_MESSAGEFOLDERATTRIBUTE_H
#define AKONADI_MESSAGEFOLDERATTRIBUTE_H

#include "akonadi-kmime_export.h"
#include <akonadi/attribute.h>

#include <QtCore/QByteArray>

namespace Akonadi {

/**
  Message folder information. Used eg. by mail clients to decide how to display the content of such collections
  @since 4.4
*/
class AKONADI_KMIME_EXPORT MessageFolderAttribute : public Attribute
{
  public:
    /**
      Creates an empty folder attribute.
    */
    MessageFolderAttribute();

    /**
      Copy constructor.
    */
    MessageFolderAttribute( const MessageFolderAttribute &other );

    /**
      Destructor.
    */
    ~MessageFolderAttribute();

    /**
      Indicates if the folder is supposed to contain mostly outbound messages.
      In such a case mail clients display the recipient address, otherwise they
      display the sender address.

      @return true if the folder contains outbound messages
    */
    bool isOutboundFolder() const;

    /**
      Set if the folder should be considered as containing mostly outbound messages.
     */
    void setOutboundFolder(bool outbound);

    // reimpl.
    QByteArray type() const;
    MessageFolderAttribute* clone() const;
    QByteArray serialized() const;
    void deserialize( const QByteArray &data );

  private:
    class Private;
    Private * const d;
};

}

#endif
