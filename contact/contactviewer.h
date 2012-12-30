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

#ifndef AKONADI_CONTACTVIEWER_H
#define AKONADI_CONTACTVIEWER_H

#include "akonadi-contact_export.h"

#include <akonadi/itemmonitor.h>

#include <QWidget>

class KUrl;

namespace KABC {
class Address;
class Addressee;
class PhoneNumber;
}

namespace Akonadi {

class AbstractContactFormatter;

/**
 * @short A viewer component for contacts in Akonadi.
 *
 * This widgets provides a way to show a contact from the
 * Akonadi storage or a raw contact object.
 *
 * Examples:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * const Item contact = ...
 *
 * ContactViewer *viewer = new ContactViewer( this );
 * viewer->setContact( contact );
 *
 * @endcode

 * @code
 *
 * using namespace Akonadi;
 *
 * const KABC::Addressee contact = ...
 *
 * ContactViewer *viewer = new ContactViewer( this );
 * viewer->setContact( contact );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT ContactViewer : public QWidget, public Akonadi::ItemMonitor
{
  Q_OBJECT

  public:
    /**
     * Creates a new contact viewer.
     *
     * @param parent The parent widget.
     */
    ContactViewer( QWidget *parent = 0 );

    /**
     * Destroys the contact viewer.
     */
    ~ContactViewer();

    /**
     * Returns the contact that is currently displayed.
     *
     * @note The returned contact is only valid if it was
     *       set with setContact() before.
     */
    Akonadi::Item contact() const;

    /**
     * Returns the raw contact that is currently displayed.
     *
     * @since 4.5
     */
    KABC::Addressee rawContact() const;

    /**
     * Sets the contact @p formatter that should be used for formatting the
     * contact. If formatter is @c 0, the standard formatter will be used.
     * @param formatter the contact formatter to set
     * @note The contact viewer won't take ownership of the formatter.
     *
     * @since 4.6
     */
    void setContactFormatter( AbstractContactFormatter *formatter );

  public Q_SLOTS:
    /**
     * Sets the @p contact that shall be displayed in the viewer.
     */
    void setContact( const Akonadi::Item &contact );

    /**
     * Sets the raw @p contact object that shall be displayed in the viewer.
     * @param contact the contact object to set
     * @since 4.5
     */
    void setRawContact( const KABC::Addressee &contact );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the user has clicked on
     * a url (e.g. homepage or blog url) in the viewer.
     *
     * @param url The url that has been clicked.
     */
    void urlClicked( const KUrl &url );

    /**
     * This signal is emitted whenever the user has clicked on
     * an email address in the viewer.
     *
     * @param name The name of the contact.
     * @param email The plain email address of the contact.
     */
    void emailClicked( const QString &name, const QString &email );

    /**
     * This signal is emitted whenever the user has clicked on a
     * phone number (that includes fax numbers as well) in the viewer.
     *
     * @param number The corresponding phone number.
     */
    void phoneNumberClicked( const KABC::PhoneNumber &number );

    /**
     * This signal is emitted whenever the user has clicked on a SMS link of a
     * phone number in the viewer.
     *
     * @param number The corresponding phone number.
     */
    void smsClicked( const KABC::PhoneNumber &number );

    /**
     * This signal is emitted whenever the user has clicked on an
     * address in the viewer.
     *
     * @param address The corresponding address.
     */
    void addressClicked( const KABC::Address &address );

  private:
    /**
     * This method is called whenever the displayed contact has been changed.
     */
    virtual void itemChanged( const Item &contact );

    /**
     * This method is called whenever the displayed contact has been
     * removed from Akonadi.
     */
    virtual void itemRemoved();

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void slotMailClicked( const QString&, const QString& ) )
    Q_PRIVATE_SLOT( d, void slotUrlClicked( const QString& ) )
    Q_PRIVATE_SLOT( d, void slotParentCollectionFetched( KJob* ) )
    //@endcond
};

}

#endif
