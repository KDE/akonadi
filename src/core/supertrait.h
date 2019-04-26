/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SUPERTRAIT_H
#define AKONADI_SUPERTRAIT_H

namespace Akonadi
{

namespace Internal {
template<typename T> struct check_type{ typedef void type; };
}

/**
  @internal
  @see SuperClass
*/
template <typename Super, typename = void>
struct SuperClassTrait {
    typedef Super Type;
};

template <typename Class>
struct SuperClassTrait<Class, typename Internal::check_type<typename Class::SuperClass>::type> {
    typedef typename Class::SuperClass Type;
};

/**
  Type trait to provide information about a base class for a given class.
  Used eg. for the Akonadi payload mechanism.

  To provide base class introspection for own types, extend this trait as follows:
  @code
  namespace Akonadi
  {
    template <> struct SuperClass<MyClass> : public SuperClassTrait<MyBaseClass>{};
  }
  @endcode

  Alternatively, define a typedef "SuperClass" in your type, pointing to the base class.
  This avoids having to include this header file if that's inconvenient from a dependency
  point of view.
*/
template <typename Class> struct SuperClass : public SuperClassTrait<Class> {};
}

#endif
