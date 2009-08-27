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

#ifndef AKONADI_LOCALFOLDERATTRIBUTE_H
#define AKONADI_LOCALFOLDERATTRIBUTE_H

#include "akonadi-kmime_export.h"

#include <akonadi/attribute.h>

namespace Akonadi {

/**
  Attribute storing the LocalFolder type of a collection (e.g. Outbox,
  Drafts, SentMail).  All collections registered with LocalFolders must
  have this attribute.

  @author Constantin Berzan <exit3219@gmail.com>
  @since 4.4
*/
class AKONADI_KMIME_EXPORT LocalFolderAttribute : public Akonadi::Attribute
{
  public:
    /**
      Creates a new LocalFolderAttribute.
    */
    explicit LocalFolderAttribute( int type = -1 );

    /**
      Destroys the LocalFolderAttribute.
    */
    virtual ~LocalFolderAttribute();

    /* reimpl */
    virtual LocalFolderAttribute* clone() const;
    virtual QByteArray type() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

    /**
      Returns the LocalFolder type of the collection.
    */
    int folderType() const;

    /**
      Sets the LocalFolder type of the collection.
    */
    void setFolderType( int type );

  private:
    class Private;
    Private *const d;

};

} // namespace Akonadi

#endif // AKONADI_LOCALFOLDERATTRIBUTE_H
