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

#ifndef AKONADI_STANDARDCONTACTFORMATTER_H
#define AKONADI_STANDARDCONTACTFORMATTER_H

#include "akonadi-contact_export.h"

#include "abstractcontactformatter.h"

namespace Akonadi {

/**
 * @short A class that formats a contact as HTML code.
 *
 * Examples:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * const KABC::Addressee contact = ...
 *
 * StandardContactFormatter formatter;
 * formatter.setContact( contact );
 *
 * QTextBrowser *view = new QTextBrowser;
 * view->setHtml( formatter.toHtml() );
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_CONTACT_EXPORT StandardContactFormatter : public AbstractContactFormatter
{
  public:
    /**
     * Creates a new standard contact formatter.
     */
    StandardContactFormatter();

    /**
     * Destroys the standard contact formatter.
     */
    virtual ~StandardContactFormatter();

    /**
     * Returns the contact formatted as HTML.
     */
    virtual QString toHtml( HtmlForm form = SelfcontainedForm ) const;

    /*
     * @since 4.9.1
     */
    void setDisplayQRCode( bool show );
    /*
     * @since 4.9.1
     */
    bool displayQRCode() const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
