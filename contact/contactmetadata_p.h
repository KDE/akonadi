/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef AKONADI_CONTACTMETADATA_H
#define AKONADI_CONTACTMETADATA_H

#include <QtCore/QStringList>

namespace Akonadi
{

class Item;

/**
 * @short A helper class for storing contact specific settings.
 */
class ContactMetaData
{
  public:
    /**
     * Creates a contact meta data object.
     */
    ContactMetaData();

    /**
     * Destroys the contact meta data object.
     */
    ~ContactMetaData();

    /**
     * Loads the meta data for the given @p contact.
     */
    void load( const Akonadi::Item &contact );

    /**
     * Stores the meta data to the given @p contact.
     */
    void store( Akonadi::Item &contact );

    /**
     * Sets the mode that is used for the display
     * name of that contact.
     */
    void setDisplayNameMode( int mode );

    /**
     * Returns the mode that is used for the display
     * name of that contact.
     */
    int displayNameMode() const;

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( ContactMetaData )

    class Private;
    Private* const d;
    //@endcond
};

}

#endif
