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

#ifndef CONTACTEDITOR_H
#define CONTACTEDITOR_H

#include "abstractcontacteditorwidget_p.h"

namespace KABC
{
class Addressee;
}

/**
 * @short A widget for editing a contact.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ContactEditorWidget : public Akonadi::AbstractContactEditorWidget
{
public:
    enum DisplayMode {
        FullMode, ///< Show all pages
        VCardMode ///< Show just pages with elements stored in vcard.
    };

    /**
     * Creates a new contact editor widget.
     *
     * @param parent The parent widget.
     */
    explicit ContactEditorWidget(QWidget *parent = 0);

    ContactEditorWidget(DisplayMode displayMode, QWidget *parent);

    /**
     * Destroys the contact editor widget.
     */
    ~ContactEditorWidget();

    /**
     * Initializes the fields of the contact editor
     * with the values from a @p contact.
     */
    void loadContact(const KABC::Addressee &contact, const Akonadi::ContactMetaData &metaData);

    /**
     * Stores back the fields of the contact editor
     * into the given @p contact.
     */
    void storeContact(KABC::Addressee &contact, Akonadi::ContactMetaData &metaData) const;

    /**
     * Sets whether the contact in the editor allows
     * the user to edit the contact or not.
     */
    void setReadOnly(bool readOnly);

private:
    class Private;
    Private *const d;
};

#endif
