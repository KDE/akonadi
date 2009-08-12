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

#ifndef AKONADI_CONTACTEDITOR_H
#define AKONADI_CONTACTEDITOR_H

#include "akonadi-contact_export.h"

#include <QtGui/QWidget>

namespace Akonadi {

class AbstractContactEditorWidget;
class Collection;
class Item;

/**
 * @short An widget to edit contacts in Akonadi.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT ContactEditor : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Describes the mode of the editor.
     */
    enum Mode
    {
      CreateMode, ///< Creates a new contact
      EditMode    ///< Edits an existing contact
    };

    /**
     * Creates a new contact editor with the standard editor widget.
     *
     * @param mode The mode of the editor.
     * @param editorWidget The contact editor widget that shall be used for editing.
     * @param parent The parent widget of the editor.
     */
    explicit ContactEditor( Mode mode, QWidget *parent = 0 );

    /**
     * Creates a new contact editor with a custom editor widget.
     *
     * @param mode The mode of the editor.
     * @param editorWidget The contact editor widget that shall be used for editing.
     * @param parent The parent widget of the editor.
     */
    explicit ContactEditor( Mode mode, AbstractContactEditorWidget *editorWidget, QWidget *parent = 0 );

    /**
     * Destroys the contact editor.
     */
    virtual ~ContactEditor();

  public Q_SLOTS:
    /**
     * Loads the @p contact into the editor.
     */
    void loadContact( const Akonadi::Item &contact );

    /**
     * Saves the contact from the editor back to the storage.
     */
    void saveContact();

    /**
     * Sets the @p collection which shall be used to store new
     * contacts.
     */
    void setDefaultCollection( const Akonadi::Collection &collection );

  Q_SIGNALS:
    /**
     * This signal is emitted when the @p contact has been saved back
     * to the storage.
     */
    void contactStored( const Akonadi::Item &contact );

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

    Q_PRIVATE_SLOT( d, void fetchDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void storeDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
    //@endcond PRIVATE
};

}

#endif
