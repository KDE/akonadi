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

/**
 * @short The interface for all contact formatters.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
//AK_REVIEW: extend api docs (usage), move class to libkabc
class AKONADI_CONTACT_EXPORT AbstractContactFormatter
{
  public:
    /**
     * Describes the form of the HTML that is created.
     */
    enum HtmlForm
    {
      SelfcontainedForm,                ///< Creates a complete HTML document
      EmbeddableForm,                   ///< Creates a div HTML element that can be embedded.
      UserForm = SelfcontainedForm + 42 ///< Point for extension
    };

    /**
     * Destroys the abstract contact formatter.
     */
    virtual ~AbstractContactFormatter();

    /**
     * Sets the @p contact that shall be formatted.
     */
    //AK_REVIEW: add getter and make it non-abstract
    virtual void setContact( const KABC::Addressee &contact ) = 0;

    //AK_REVIEW: add method setItem( const Akonadi::Item ) and getter

    /**
     * Sets the custom field @p descriptions that shall be used.
     */
    //AK_REVIEW: change QVariantList to QList<QMap<QString, QVariant>>
    //AK_REVIEW: add getter and make it non-abstract
    virtual void setCustomFieldDescriptions( const QVariantList &descriptions ) = 0;

    /**
     * Returns the contact formatted as HTML
     */
    virtual QString toHtml( HtmlForm form = SelfcontainedForm ) const = 0;
};

}

#endif
