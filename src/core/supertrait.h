/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

namespace Akonadi
{
namespace Internal
{
template<typename T> struct check_type {
    using type = void;
};
}

/**
  @internal
  @see SuperClass
*/
template<typename Super, typename = void> struct SuperClassTrait {
    using Type = Super;
};

template<typename Class> struct SuperClassTrait<Class, typename Internal::check_type<typename Class::SuperClass>::type> {
    using Type = typename Class::SuperClass;
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
template<typename Class> struct SuperClass : public SuperClassTrait<Class> {
};
}

