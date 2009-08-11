/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef AKONADI_ADDRESSATTRIBUTE_H
#define AKONADI_ADDRESSATTRIBUTE_H

#include "akonadi-kmime_export.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <akonadi/attribute.h>

namespace MailTransport {
  class Transport;
}

namespace Akonadi {

/**
  Attribute storing the From, To, Cc, Bcc addresses of a message.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT AddressAttribute : public Akonadi::Attribute
{
  public:
    /**
      Creates a new AddressAttribute.
    */
    explicit AddressAttribute( const QString &from = QString(),
                               const QStringList &to = QStringList(),
                               const QStringList &cc = QStringList(),
                               const QStringList &bcc = QStringList() );
    /**
      Destroys the AddressAttribute.
    */
    virtual ~AddressAttribute();

    
    /* reimpl */
    virtual AddressAttribute* clone() const;
    virtual QByteArray type() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    /**
      Returns the address of the sender.
    */
    QString from() const;

    /**
      Sets the address of the sender.
    */
    void setFrom( const QString &from );

    /**
      Returns the addresses of the "To:" receivers.
    */
    QStringList to() const;
    
    /**
      Sets the addresses of the "To:" receivers."
    */
    void setTo( const QStringList &to );

    /**
      Returns the addresses of the "Cc:" receivers.
    */
    QStringList cc() const;

    /**
      Sets the addresses of the "Cc:" receivers."
    */
    void setCc( const QStringList &cc );

    /**
      Returns the addresses of the "Bcc:" receivers.
    */
    QStringList bcc() const;

    /**
      Sets the addresses of the "Bcc:" receivers."
    */
    void setBcc( const QStringList &bcc );

  private:
    class Private;
    Private *const d;

};

} // namespace Akonadi

#endif // AKONADI_ADDRESSATTRIBUTE_H
