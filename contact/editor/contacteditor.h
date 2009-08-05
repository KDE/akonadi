/*
    This file is part of KAddressBook.

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

#ifndef CONTACTEDITOR_H
#define CONTACTEDITOR_H

#include "abstractcontacteditorwidget.h"

namespace KABC
{
class Addressee;
}

/**
 * @short A widget for editing a contact.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ContactEditor : public AbstractContactEditorWidget
{
  public:
    /**
     * Creates a new contact editor.
     *
     * @param parent The parent widget.
     */
    ContactEditor( QWidget *parent = 0 );

    /**
     * Destroys the contact editor.
     */
    ~ContactEditor();

    /**
     * Initializes the fields of the contact editor
     * with the values from a @p contact.
     */
    void loadContact( const KABC::Addressee &contact );

    /**
     * Stores back the fields of the contact editor
     * into the given @p contact.
     */
    void storeContact( KABC::Addressee &contact ) const;

  private:
    class Private;
    Private* const d;
};

#endif
