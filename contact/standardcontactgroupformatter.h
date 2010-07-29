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

#ifndef AKONADI_STANDARDCONTACTGROUPFORMATTER_H
#define AKONADI_STANDARDCONTACTGROUPFORMATTER_H

#include "akonadi-contact_export.h"

#include "abstractcontactgroupformatter.h"

namespace Akonadi {

/**
 * @short A class that formats a contact group as HTML code.
 *
 * Examples:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * const KABC::ContactGroup group = ...
 *
 * StandardContactGroupFormatter formatter;
 * formatter.setContactGroup( group );
 *
 * QTextBrowser *view = new QTextBrowser;
 * view->setHtml( formatter.toHtml() );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.6
 */
class AKONADI_CONTACT_EXPORT StandardContactGroupFormatter : public AbstractContactGroupFormatter
{
  public:
    /**
     * Creates a new standard contact group formatter.
     */
    StandardContactGroupFormatter();

    /**
     * Destroys the standard contact group formatter.
     */
    virtual ~StandardContactGroupFormatter();

    /**
     * Returns the contact group formatted as HTML.
     */
    virtual QString toHtml( HtmlForm form = SelfcontainedForm ) const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
