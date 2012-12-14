/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ABSTRACTCONTACTGROUPFORMATTER_H
#define AKONADI_ABSTRACTCONTACTGROUPFORMATTER_H

#include "akonadi-contact_export.h"

#include <QtCore/QVariant>

namespace KABC {
class ContactGroup;
}

namespace Akonadi {
class Item;

/**
 * @short The interface for all contact group formatters.
 *
 * This is the interface that can be used to format an Akonadi
 * item with a contact group payload or a contact group itself as HTML.
 *
 * @see StandardContactGroupFormatter
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.6
 */
class AKONADI_CONTACT_EXPORT AbstractContactGroupFormatter
{
  public:
    /**
     * Describes the form of the HTML that is created.
     */
    enum HtmlForm {
      SelfcontainedForm,                ///< Creates a complete HTML document
      EmbeddableForm,                   ///< Creates a div HTML element that can be embedded.
      UserForm = SelfcontainedForm + 42 ///< Point for extension
    };

    /**
     * Creates a new abstract contact group formatter.
     */
    AbstractContactGroupFormatter();

    /**
     * Destroys the abstract contact group formatter.
     */
    virtual ~AbstractContactGroupFormatter();

    /**
     * Sets the contact @p group that will be formatted.
     */
    void setContactGroup( const KABC::ContactGroup &group );

    /**
     * Returns the contact group that will be formatted.
     */
    KABC::ContactGroup contactGroup() const;

    /**
     * Sets the @p item who's payload will be formatted.
     *
     * @note The payload must be a valid KABC::ContactGroup object.
     *
     * @param item item, who's payload will be formatted.
     */
    void setItem( const Akonadi::Item &item );

    /**
     * Returns the item who's payload will be formatted.
     */
    Akonadi::Item item() const;

    /**
     * Sets the additional @p fields that will be shown.
     *
     * The fields list contains a QVariantMap for each additional field
     * with the following keys:
     *   - key   (string) The identifier of the field
     *   - title (string) The i18n'ed title of the field
     *   - value (string) The value of the field
     *
     * @param fields additional fields that will be shown
     */
    void setAdditionalFields( const QList<QVariantMap> &fields );

    /**
     * Returns the additional fields that will be shown.
     */
    QList<QVariantMap> additionalFields() const;

    /**
     * This method must be reimplemented to return the contact group formatted as HTML
     * according to the requested @p form.
     */
    virtual QString toHtml( HtmlForm form = SelfcontainedForm ) const = 0;

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( AbstractContactGroupFormatter )

    class Private;
    Private* const d;
    //@endcond
};

}

#endif
