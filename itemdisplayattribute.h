/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_ITEMDISPLAYATTRIBUTE_H
#define AKONADI_ITEMDISPLAYATTRIBUTE_H

#include <akonadi/attribute.h>

class KIcon;

namespace Akonadi {

/**
 * Display properties of an item, such as translated names and icons.
 * @since 4.2
 */
class AKONADI_EXPORT ItemDisplayAttribute : public Attribute
{
  public:
    /**
     * Creates a new item display attribute.
     */
    ItemDisplayAttribute();

    /**
     * Destructor.
     */
    ~ItemDisplayAttribute();

    /**
     * Returns the name that should be used for display.
     */
    QString displayName() const;

    /**
     * Set the display name.
     */
    void setDisplayName( const QString &name );

    /**
     * Returns the icon that should be used for this item.
     */
    KIcon icon() const;

    /**
     * Returns the icon name of the icon returned by icon().
     */
    QString iconName() const;

    /**
     * Set the icon name for the items default icon.
     */
    void setIconName( const QString &icon );

    /* reimpl */
    QByteArray type() const;
    ItemDisplayAttribute* clone() const;
    QByteArray serialized() const;
    void deserialize( const QByteArray &data );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
