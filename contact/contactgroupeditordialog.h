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

#ifndef AKONADI_CONTACTGROUPEDITORDIALOG_H
#define AKONADI_CONTACTGROUPEDITORDIALOG_H

#include "akonadi-contact_export.h"

#include <kdialog.h>

class QAbstractItemModel;

namespace Akonadi {

class Item;
class ContactGroupEditor;

/**
 * @short A dialog for editing a contact group.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT ContactGroupEditorDialog : public KDialog
{
  Q_OBJECT

  public:
    /**
     * Describes the mode of the contact group editor.
     */
    enum Mode
    {
      CreateMode, ///< Creates a new contact group
      EditMode    ///< Edits an existing contact group
    };

    /**
     * Creates a new contact group editor dialog.
     *
     * @param mode The mode of the dialog.
     * @param parent The parent widget of the dialog.
     */
    ContactGroupEditorDialog( Mode mode, QWidget *parent = 0 );

    /**
     * Destroys the contact group editor dialog.
     */
    ~ContactGroupEditorDialog();

    /**
     * Sets the contact @p group to edit when in EditMode.
     */
    void setContactGroup( const Akonadi::Item &group );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever a contact group was updated or stored.
     *
     * @param group The contact group.
     */
    void contactGroupStored( const Akonadi::Item &group );

  protected Q_SLOTS:
    virtual void slotButtonClicked( int button );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
