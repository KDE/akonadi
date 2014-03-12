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

#ifndef AKONADI_SHOWADDRESSACTION_H
#define AKONADI_SHOWADDRESSACTION_H

namespace KABC {
class Address;
}

namespace Akonadi {

/**
 * @short A contact action to show the address of a contact on a map.
 *
 * This class provides the functionality to show the postal address
 * of a contact on a map in a webbrowser.
 *
 * Example:
 *
 * @code
 *
 * const KABC::Addressee contact = ...
 *
 * const KABC::Address::List addresses = contact.addresses();
 * if ( !addresses.isEmpty() ) {
 *   Akonadi::ShowAddressAction action;
 *   action.showAddress( addresses.first() );
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class ShowAddressAction
{
public:
    void showAddress(const KABC::Address &address);
};

}

#endif
