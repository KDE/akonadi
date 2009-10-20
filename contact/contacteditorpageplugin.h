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

#ifndef AKONADI_CONTACTEDITORPAGEPLUGIN_H
#define AKONADI_CONTACTEDITORPAGEPLUGIN_H

#include <QtGui/QWidget>

namespace KABC {
class Addressee;
}

namespace Akonadi
{

/**
 * @short The base class for custom ContactEditor page plugins.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class ContactEditorPagePlugin : public QWidget
{
  public:
    /**
     * Returns the i18n'd page title.
     */
    virtual QString title() const = 0;

    /**
     * This method is called to fill the editor widget with the data from @p contact.
     */
    virtual void loadContact( const KABC::Addressee &contact ) = 0;

    /**
     * This method is called to store the data from the editor widget into @p contact.
     */
    virtual void storeContact( KABC::Addressee &contact ) const = 0;

    /**
     * This method is called to set the editor widget @p readOnly.
     */
    virtual void setReadOnly( bool readOnly ) = 0;
};

}

Q_DECLARE_INTERFACE( Akonadi::ContactEditorPagePlugin, "org.freedesktop.Akonadi.ContactEditorPagePlugin/1.0" )

#endif
