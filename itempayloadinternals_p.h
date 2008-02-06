/*
    Copyright (c) 2007 Till Adam <adam@kde.org>

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

#ifndef ITEMPAYLOADINTERNALS_P_H
#define ITEMPAYLOADINTERNALS_P_H

//@cond PRIVATE

#include <QtCore/QtGlobal>

#include <typeinfo>

/* WARNING
 * The below is an implementation detail of the Item class. It is not to be
 * considered public API, and subject to change without notice
 */

struct PayloadBase
{
    virtual ~PayloadBase() { }
    virtual PayloadBase * clone() const = 0;
    virtual const char* typeName() const = 0;
};

template <typename T>
struct Payload : public PayloadBase
{
    Payload( T p ) { payload = p; }
    Payload( const Payload& other )
    {
       payload = other.payload;
    }
    Payload & operator=( const Payload & other )
    {
       payload = other.payload;
    }

    PayloadBase * clone() const
    {
        return new Payload<T>( const_cast<Payload<T>* >(this)->payload);
    }

    const char* typeName() const
    {
      return typeid(const_cast<Payload<T>*> (this)).name();
    }

    T payload;
};

template <typename T>
struct Payload<T*> : public PayloadBase
{
    Payload( T* )
    {
        Q_ASSERT_X( false, "Akonadi::Payload", "The Item class is not intended to be used with raw pointer types. Please use a smart pointer instead." );
    }
};
//@endcond

#endif

