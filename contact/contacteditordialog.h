/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_CONTACTEDITORDIALOG_H
#define AKONADI_CONTACTEDITORDIALOG_H

#include "akonadi-contact_export.h"

#include <kdialog.h>

class QAbstractItemModel;

namespace Akonadi {

class AbstractContactEditorWidget;
class Item;

/**
 * @short A dialog for creating or editing a contact in Akonadi.
 *
 * This dialog provides a way to create a new contact or edit
 * an existing contact in Akonadi.
 *
 * Example for creating a new contact:
 *
 * @code
 *
 * Akonadi::ContactEditorDialog dlg( Akonadi::ContactEditorDialog::CreateMode, this );
 *
 * if ( dlg.exec() ) {
 *   qDebug() << "New contact has been added to the address book";
 * } else {
 *   qDebug() << "User has canceled operation";
 * }
 *
 * @endcode
 *
 * Example for editing an existing contact:
 *
 * @code
 *
 * const Akonadi::Item contact = ...;
 *
 * Akonadi::ContactEditorDialog dlg( Akonadi::ContactEditorDialog::EditMode, this );
 * dlg.setContact( contact );
 *
 * if ( dlg.exec() ) {
 *   qDebug() << "Contact has been edited";
 * } else {
 *   qDebug() << "User has canceled operation";
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT ContactEditorDialog : public KDialog
{
  Q_OBJECT

  public:
    /**
     * Describes the mode of the editor dialog.
     */
    enum Mode
    {
      CreateMode, ///< Creates a new contact
      EditMode    ///< Edits an existing contact
    };

    /**
     * Creates a new contact editor dialog with the standard editor widget.
     *
     * @param mode The mode of the dialog.
     * @param parent The parent widget of the dialog.
     */
    ContactEditorDialog( Mode mode, QWidget *parent = 0 );

    /**
     * Creates a new contact editor dialog with a custom editor widget.
     *
     * @param mode The mode of the dialog.
     * @param collectionModel The collection model that is used to allow the user
     *                        to select a target collection for new contacts.
     * @param editorWidget The contact editor widget that shall be used for editing.
     * @param parent The parent widget of the dialog.
     */
    ContactEditorDialog( Mode mode, AbstractContactEditorWidget *editorWidget, QWidget *parent = 0 );

    /**
     * Destroys the contact editor dialog.
     */
    ~ContactEditorDialog();

    /**
     * Sets the @p contact to edit when in EditMode.
     *
     * @note The contact item just must have a uid set, all
     *       other data are fetched by the dialog automatically.
     */
    void setContact( const Akonadi::Item &contact );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever a contact was updated or stored.
     *
     * @param contact The data reference of the contact.
     */
    void contactStored( const Akonadi::Item &contact );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotOkClicked() )
    Q_PRIVATE_SLOT( d, void slotCancelClicked() )
    //@endcond PRIVATE
};

}

#endif
