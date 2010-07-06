/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_EMAILADDRESSSELECTION_H
#define AKONADI_EMAILADDRESSSELECTION_H

#include "akonadi-contact_export.h"

#include <QtCore/QList>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QString>

namespace Akonadi {

class Item;

/**
 * @short An selection of an email address and corresponding name.
 *
 * This class encapsulates the selection of an email address and name
 * as it is returned by EmailAddressSelectionWidget or EmailAddressSelectionDialog.
 *
 * It offers convenience methods to retrieve the quoted version of the
 * email address and access to the Akonadi item that is associated with
 * this address.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_CONTACT_EXPORT EmailAddressSelection
{
  public:
    /**
     * A list of email address selection objects.
     */
    typedef QList<EmailAddressSelection> List;

    /**
     * Creates a new email address selection.
     */
    EmailAddressSelection();

    /**
     * Creates a new email address selection from an @p other selection.
     */
    EmailAddressSelection( const EmailAddressSelection &other );

    /**
     * Replaces this email address selection with the @p other selection.
     */
    EmailAddressSelection &operator=( const EmailAddressSelection &other );

    /**
     * Destroys the email address selection.
     */
    ~EmailAddressSelection();

    /**
     * Returns whether the selection is valid.
     */
    bool isValid() const;

    /**
     * Returns the name that is associated with the selected email address.
     */
    QString name() const;

    /**
     * Returns the address part of the selected email address.
     *
     * @note If a contact group has been selected, the name of the contact
     *       group is returned here and must be expanded by the caller.
     */
    QString email() const;

    /**
     * Returns the name and email address together, properly quoted if needed.
     *
     * @note If a contact group has been selected, the name of the contact
     *       group is returned here and must be expanded by the caller.
     */
    QString quotedEmail() const;

    /**
     * Returns the Akonadi item that is associated with the selected email address.
     */
    Akonadi::Item item() const;

  private:
    //@cond PRIVATE
    friend class EmailAddressSelectionWidget;

    class Private;
    QSharedDataPointer<Private> d;
    //@endcond
};

}

Q_DECLARE_TYPEINFO( Akonadi::EmailAddressSelection, Q_MOVABLE_TYPE );

#endif
