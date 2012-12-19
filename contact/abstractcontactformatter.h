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

#ifndef AKONADI_ABSTRACTCONTACTFORMATTER_H
#define AKONADI_ABSTRACTCONTACTFORMATTER_H

#include "akonadi-contact_export.h"

#include <QtCore/QVariantMap>

namespace KABC {
class Addressee;
}

namespace Akonadi {
class Item;

/**
 * @short The interface for all contact formatters.
 *
 * This is the interface that can be used to format an Akonadi
 * item with a contact payload or a contact itself as HTML.
 *
 * @see StandardContactFormatter
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_CONTACT_EXPORT AbstractContactFormatter
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
     * Creates a new abstract contact formatter.
     */
    AbstractContactFormatter();

    /**
     * Destroys the abstract contact formatter.
     */
    virtual ~AbstractContactFormatter();

    /**
     * Sets the @p contact that will be formatted.
     * @param contact contact to be formatted
     */
    void setContact( const KABC::Addressee &contact );

    /**
     * Returns the contact that will be formatted.
     */
    KABC::Addressee contact() const;

    /**
     * Sets the @p item who's payload will be formatted.
     *
     * @note The payload must be a valid KABC::Addressee object.
     * @param item item, who's payload will be formatted.
     */
    void setItem( const Akonadi::Item &item );

    /**
     * Returns the item who's payload will be formatted.
     */
    Akonadi::Item item() const;

    /**
     * Sets the custom field @p descriptions that will be used.
     *
     * The description list contains a QVariantMap for each custom field
     * with the following keys:
     *   - key   (string) The identifier of the field
     *   - title (string) The i18n'ed title of the field
     *   - type  (string) The type description of the field
     *     Possible values for type description are
     *       - text
     *       - numeric
     *       - boolean
     *       - date
     *       - time
     *       - datetime
     *
     * @param descriptions list with field descriptions
     */
    void setCustomFieldDescriptions( const QList<QVariantMap> &descriptions );

    /**
     * Returns the custom field descriptions that will be used.
     */
    QList<QVariantMap> customFieldDescriptions() const;

    /**
     * This method must be reimplemented to return the contact formatted as HTML
     * according to the requested @p form.
     * @param form how to render the contact into HTML
     */
    virtual QString toHtml( HtmlForm form = SelfcontainedForm ) const = 0;

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( AbstractContactFormatter )

    class Private;
    Private* const d;
    //@endcond
};

}

#endif
