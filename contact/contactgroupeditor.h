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

#ifndef AKONADI_CONTACTGROUPEDITOR_H
#define AKONADI_CONTACTGROUPEDITOR_H

#include "akonadi-contact_export.h"

#include <QtGui/QWidget>

namespace KABC {
class ContactGroup;
}

namespace Akonadi {

class Collection;
class Item;

/**
 * @short An widget to edit contact groups in Akonadi.
 *
 * This widget provides a way to create a new contact group or edit
 * an existing contact group in Akonadi.
 *
 * Example for creating a new contact group:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * ContactGroupEditor *editor = new ContactGroupEditor( Akonadi::ContactGroupEditor::CreateMode, this );
 *
 * ...
 *
 * if ( !editor->saveContactGroup() ) {
 *   qDebug() << "Unable to save new contact group to storage";
 *   return;
 * }
 *
 * @endcode
 *
 * Example for editing an existing contact group:
 *
 * @code
 *
 * const Akonadi::Item contactGroup = ...;
 *
 * ContactGroupEditor *editor = new ContactGroupEditor( Akonadi::ContactGroupEditor::EditMode, this );
 * editor->loadContactGroup( contactGroup );
 *
 * ...
 *
 * if ( !editor->saveContactGroup() ) {
 *   qDebug() << "Unable to save changed contact group to storage";
 *   return;
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT ContactGroupEditor : public QWidget
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
     * Creates a new contact group editor.
     *
     * @param mode The mode of the editor.
     * @param parent The parent widget of the editor.
     */
    explicit ContactGroupEditor( Mode mode, QWidget *parent = 0 );

    /**
     * Destroys the contact group editor.
     */
    virtual ~ContactGroupEditor();

    /**
     * Sets a contact @p group that is used as template in create mode.
     *
     * The fields of the editor will be prefilled with the content
     * of the group.
     */
    void setContactGroupTemplate( const KABC::ContactGroup &group );

    /**
     * Sets the @p addressbook which shall be used to store new
     * contact groups.
     */
    void setDefaultAddressBook( const Akonadi::Collection &addressbook );

  public Q_SLOTS:
    /**
     * Loads the contact @p group into the editor.
     */
    void loadContactGroup( const Akonadi::Item &group );

    /**
     * Saves the contact group from the editor back to the storage.
     *
     * @returns @c true if the contact group has been saved successfully, false otherwise.
     */
    bool saveContactGroup();

  Q_SIGNALS:
    /**
     * This signal is emitted when the contact @p group has been saved back
     * to the storage.
     */
    void contactGroupStored( const Akonadi::Item &group );

    /**
     * This signal is emitted when an error occurred during the save.
     *
     * @p errorMsg The error message.
     */
    void error( const QString &errorMsg );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_DISABLE_COPY( ContactGroupEditor )

    Q_PRIVATE_SLOT( d, void itemFetchDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void parentCollectionFetchDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void storeDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
    Q_PRIVATE_SLOT( d, void adaptHeaderSizes() )
    //@endcond
};

}

#endif
