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

#ifndef AKONADI_CONTACTGROUPVIEWERDIALOG_H
#define AKONADI_CONTACTGROUPVIEWERDIALOG_H

#include "akonadi-contact_export.h"

#include <kdialog.h>

namespace Akonadi {

class Item;

class ContactGroupViewer;

/**
 * @short A dialog for displaying a contact group in Akonadi.
 *
 * This dialog provides a way to show a contact group from the
 * Akonadi storage.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * const Item group = ...
 *
 * ContactGroupViewerDialog *dlg = new ContactGroupViewerDialog( this );
 * dlg->setContactGroup( group );
 * dlg->show();
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT ContactGroupViewerDialog : public KDialog
{
  Q_OBJECT

  public:
    /**
     * Creates a new contact group viewer dialog.
     *
     * @param parent The parent widget of the dialog.
     */
    ContactGroupViewerDialog( QWidget *parent = 0 );

    /**
     * Destroys the contact group viewer dialog.
     */
    ~ContactGroupViewerDialog();

    /**
     * Returns the contact group that is currently displayed.
     */
    Akonadi::Item contactGroup() const;

    /**
     * Returns the ContactGroupViewer that is used by this dialog.
     */
    ContactGroupViewer *viewer() const;

  public Q_SLOTS:
    /**
     * Sets the contact @p group that shall be displayed in the dialog.
     */
    void setContactGroup( const Akonadi::Item &group );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
