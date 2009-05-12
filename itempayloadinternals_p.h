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

#include <kpimutils/supertrait.h>

#include <QtCore/QtGlobal>
#include <QtCore/QSharedPointer>

#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_same.hpp>
#include <typeinfo>

#include "exception.h"

/* WARNING
 * The below is an implementation detail of the Item class. It is not to be
 * considered public API, and subject to change without notice
 */

namespace Akonadi {
namespace Internal {

/**
  @internal
  Payload type traits. Implements specialized handling for polymorphic types and smart pointers.
  The default one is never used (as isPolymorphic is always false) and only contains safe dummy
  implementations to make the compiler happy (in practice it will always optimized away anyway).
*/
template <typename T> struct PayloadTrait
{
  /// type of the payload object contained inside a shared pointer
  typedef T ElementType;
  /// type of the base class of the payload object inside a shared pointer,
  /// same as ElementType if there is no super class
  typedef typename KPIMUtils::SuperClass<T>::Type SuperElementType;
  /// type of this payload object
  typedef T Type;
  /// type of the payload to store a base class of this payload
  /// (eg. a shared pointer containing a pointer to SuperElementType)
  /// same as Type if there is not super class
  typedef typename KPIMUtils::SuperClass<T>::Type SuperType;
  /// indicates if this payload is polymorphic, that is is a shared pointer
  /// and has a known super class
  static const bool isPolymorphic = false;
  /// checks an object of this payload type for being @c null
  static inline bool isNull( const Type & ) { return true; }
  /// casts to Type from @c U
  /// throws a PayloadException if casting failed
  template <typename U> static inline Type castFrom( const U& )
  {
    throw PayloadException( "you should never get here" );
  }
  /// tests if casting from @c U to Type is possible
  template <typename U> static inline bool canCastFrom( const U& )
  {
    return false;
  }
  /// cast to @c U from Type
  template <typename U> static inline U castTo( const Type& )
  {
    throw PayloadException( "you should never get here" );
  }
};

/**
  @internal
  Payload type trait specialization for boost::shared_ptr
  for documentation of the various members, see above
*/
template <typename T> struct PayloadTrait<boost::shared_ptr<T> >
{
  typedef T ElementType;
  typedef typename KPIMUtils::SuperClass<T>::Type SuperElementType;
  typedef boost::shared_ptr<T> Type;
  typedef boost::shared_ptr<typename KPIMUtils::SuperClass<T>::Type> SuperType;
  static const bool isPolymorphic = !boost::is_same<ElementType, SuperElementType>::value;
  static inline bool isNull( const Type &p ) { return p.get() == 0; }
  template <typename U> static inline Type castFrom( const boost::shared_ptr<U> &p )
  {
    const Type sp = boost::dynamic_pointer_cast<T,U>( p );
    if ( sp.get() != 0 || p.get() == 0 )
      return sp;
    throw PayloadException( "boost::dynamic_pointer_cast failed" );
  }
  template <typename U> static inline bool canCastFrom( const boost::shared_ptr<U> &p )
  {
    const Type sp = boost::dynamic_pointer_cast<T,U>( p );
    return sp.get() != 0 || p.get() == 0;
  }
  template <typename U> static inline boost::shared_ptr<U> castTo( const Type &p )
  {
    const boost::shared_ptr<U> sp = boost::dynamic_pointer_cast<U,T>( p );
    return sp;
  }
};

/**
  @internal
  Payload type trait specialization for QSharedPointer
  for documentation of the various members, see above
*/
template <typename T> struct PayloadTrait<QSharedPointer<T> >
{
  typedef T ElementType;
  typedef typename KPIMUtils::SuperClass<T>::Type SuperElementType;
  typedef QSharedPointer<T> Type;
  typedef QSharedPointer<typename KPIMUtils::SuperClass<T>::Type> SuperType;
  static const bool isPolymorphic = !boost::is_same<ElementType, SuperElementType>::value;
  static inline bool isNull( const Type &p ) { return p.isNull(); }
  template <typename U> static inline Type castFrom( const QSharedPointer<U> &p )
  {
    const Type sp = qSharedPointerDynamicCast<T,U>( p );
    if ( !sp.isNull() || p.isNull() )
      return sp;
    throw PayloadException( "qSharedPointerDynamicCast failed" );
  }
  template <typename U> static inline bool canCastFrom( const QSharedPointer<U> &p )
  {
    const Type sp = qSharedPointerDynamicCast<T,U>( p );
    return !sp.isNull() || p.isNull();
  }
  template <typename U> static inline QSharedPointer<U> castTo( const Type &p )
  {
    const QSharedPointer<U> sp = qSharedPointerDynamicCast<U,T>( p );
    return sp;
  }
};

}

/**
 * @internal
 * Non-template base class for the payload container.
 * @note Move to Internal namespace for KDE5
 */
struct PayloadBase
{
    virtual ~PayloadBase() { }
    virtual PayloadBase * clone() const = 0;
    virtual const char* typeName() const = 0;
};

/**
 * @internal
 * Container for the actual payload object.
 * @note Move to Internal namespace for KDE5
 */
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

/**
 * @internal
 * abstract, will therefore always fail to compile for pointer payloads
 */
template <typename T>
struct Payload<T*> : public PayloadBase
{
};

namespace Internal {

/**
  @internal
  Basically a dynamic_cast that also works across DSO boundaries.
*/
template <typename T> inline Payload<T>* payload_cast( PayloadBase* payloadBase )
{
  Payload<T> *p = dynamic_cast<Payload<T>*>( payloadBase );
  // try harder to cast, workaround for some gcc issue with template instances in multiple DSO's
  if ( !p && strcmp( payloadBase->typeName(), typeid(p).name() ) == 0 ) {
    p = static_cast<Payload<T>*>( payloadBase );
  }
  return p;
}

}

}

#endif

