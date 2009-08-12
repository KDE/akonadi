/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTMETADATAATTRIBUTE_H
#define AKONADI_CONTACTMETADATAATTRIBUTE_H

#include <akonadi/attribute.h>

#include <QtCore/QVariant>

namespace Akonadi {

/**
 * @short Attribute to store contact specific meta data.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ContactMetaDataAttribute : public Akonadi::Attribute
{
  public:
    /**
     * Creates a new contact meta data attribute.
     *
     * @param data The meta data.
     */
    ContactMetaDataAttribute( const QVariantMap &data );

    /**
     * Destroys the contact meta data attribute.
     */
    ~ContactMetaDataAttribute();

    /**
     * Sets the meta @p data.
     */
    void setMetaData( const QVariantMap &data );

    /**
     * Returns the meta data.
     */
    QVariantMap metaData() const;

    virtual QByteArray type() const;
    virtual Attribute* clone() const;
    virtual QByteArray serialized() const;
    virtual void deserialize( const QByteArray &data );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
