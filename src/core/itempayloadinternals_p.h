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

#include "supertrait.h"

#include <QtCore/QtGlobal>
#include <QtCore/QSharedPointer>
#include <QtCore/QMetaType>


#include <boost/shared_ptr.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>

#include <type_traits>
#include <typeinfo>
#include <memory>

#include "exception.h"

//@cond PRIVATE Doxygen 1.7.1 hangs processing this file. so skip it.
//for more info, see https://bugzilla.gnome.org/show_bug.cgi?id=531637

/* WARNING
 * The below is an implementation detail of the Item class. It is not to be
 * considered public API, and subject to change without notice
 */

namespace Akonadi {
namespace Internal {

template <typename T>
struct has_clone_method {
private:
    template <typename S, S * (S::*)() const> struct sfinae
    {
    };
    struct No
    {
    };
    struct Yes
    {
        No no[2];
    };
    template <typename S> static No  test(...);
    template <typename S> static Yes test(sfinae<S, &S::clone> *);
public:
    static const bool value = sizeof (test<T>(0)) == sizeof (Yes) ;
};

template <typename T, bool b>
struct clone_traits_helper {
    // runtime error (commented in) or compiletime error (commented out)?
    // ### runtime error, until we check has_clone_method in the
    // ### Item::payload<T> impl directly...
    template <typename U>
    static T *clone(U)
    {
        return 0;
    }
};

template <typename T>
struct clone_traits_helper<T, true>
{
    static T *clone(T *t)
    {
        return t ? t->clone() : 0 ;
    }
};

template <typename T>
struct clone_traits : clone_traits_helper<T, has_clone_method<T>::value>
{
};

template <typename T>
struct shared_pointer_traits
{
    static const bool defined = false;
};

template <typename T>
struct shared_pointer_traits< boost::shared_ptr<T> >
{
    static const bool defined = true;
    typedef T element_type;
    template <typename S>
    struct make {
        typedef boost::shared_ptr<S> type;
    };
    typedef QSharedPointer<T> next_shared_ptr;
};

template <typename T>
struct shared_pointer_traits< QSharedPointer<T> >
{
    static const bool defined = true;
    typedef T element_type;
    template <typename S>
    struct make {
        typedef QSharedPointer<S> type;
    };
    typedef std::shared_ptr<T> next_shared_ptr;
};

template <typename T>
struct shared_pointer_traits< std::shared_ptr<T> >
{
    static const bool defined = true;
    typedef T element_type;
    template <typename S>
    struct make {
        typedef std::shared_ptr<S> type;
    };
    typedef boost::shared_ptr<T> next_shared_ptr;
};

template <typename T>
struct is_shared_pointer
{
    static const bool value = shared_pointer_traits<T>::defined;
};

template <typename T>
struct get_hierarchy_root;

template <typename T, typename S>
struct get_hierarchy_root_recurse
    : get_hierarchy_root<S>
{
};

template <typename T>
struct get_hierarchy_root_recurse<T, T>
    : boost::mpl::identity<T>
{
};

template <typename T>
struct get_hierarchy_root
    : get_hierarchy_root_recurse< T, typename Akonadi::SuperClass<T>::Type >
{
};

template <typename T>
struct get_hierarchy_root< boost::shared_ptr<T> >
{
    typedef boost::shared_ptr< typename get_hierarchy_root<T>::type > type;
};

template <typename T>
struct get_hierarchy_root< QSharedPointer<T> >
{
    typedef QSharedPointer< typename get_hierarchy_root<T>::type > type;
};

template <typename T>
struct get_hierarchy_root< std::shared_ptr<T> >
{
    typedef std::shared_ptr< typename get_hierarchy_root<T>::type > type;
};

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
    // the metatype id for the element type, or for pointer-to-element
    // type, if in a shared pointer
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T>();
    }
    /// type of the base class of the payload object inside a shared pointer,
    /// same as ElementType if there is no super class
    typedef typename Akonadi::SuperClass<T>::Type SuperElementType;
    /// type of this payload object
    typedef T Type;
    /// type of the payload to store a base class of this payload
    /// (eg. a shared pointer containing a pointer to SuperElementType)
    /// same as Type if there is not super class
    typedef typename Akonadi::SuperClass<T>::Type SuperType;
    /// indicates if this payload is polymorphic, that is is a shared pointer
    /// and has a known super class
    static const bool isPolymorphic = false;
    /// checks an object of this payload type for being @c null
    static inline bool isNull(const Type &p)
    {
        Q_UNUSED(p);
        return true;
    }
    /// casts to Type from @c U
    /// throws a PayloadException if casting failed
    template <typename U> static inline Type castFrom(const U &)
    {
        throw PayloadException("you should never get here");
    }
    /// tests if casting from @c U to Type is possible
    template <typename U> static inline bool canCastFrom(const U &)
    {
        return false;
    }
    /// cast to @c U from Type
    template <typename U> static inline U castTo(const Type &)
    {
        throw PayloadException("you should never get here");
    }
    template <typename U> static T clone(const U &)
    {
        throw PayloadException("clone: you should never get here");
    }
    /// defines the type of shared pointer used (0: none, > 0: boost::shared_ptr, QSharedPointer, ...)
    static const unsigned int sharedPointerId = 0;
};

/**
  @internal
  Payload type trait specialization for boost::shared_ptr
  for documentation of the various members, see above
*/
template <typename T> struct PayloadTrait<boost::shared_ptr<T> >
{
    typedef T ElementType;
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T *>();
    }
    typedef typename Akonadi::SuperClass<T>::Type SuperElementType;
    typedef boost::shared_ptr<ElementType> Type;
    typedef boost::shared_ptr<SuperElementType> SuperType;
    static const bool isPolymorphic = !std::is_same<ElementType, SuperElementType>::value;
    static inline bool isNull(const Type &p)
    {
        return p.get() == 0;
    }
    template <typename U> static inline Type castFrom(const boost::shared_ptr<U> &p)
    {
        const Type sp = boost::dynamic_pointer_cast<T, U>(p);
        if (sp.get() != 0 || p.get() == 0) {
            return sp;
        }
        throw PayloadException("boost::dynamic_pointer_cast failed");
    }
    template <typename U> static inline bool canCastFrom(const boost::shared_ptr<U> &p)
    {
        const Type sp = boost::dynamic_pointer_cast<T, U>(p);
        return sp.get() != 0 || p.get() == 0;
    }
    template <typename U> static inline boost::shared_ptr<U> castTo(const Type &p)
    {
        const boost::shared_ptr<U> sp = boost::dynamic_pointer_cast<U>(p);
        return sp;
    }
    static boost::shared_ptr<T> clone(const QSharedPointer<T> &t) {
        if (T *nt = clone_traits<T>::clone(t.data())) {
            return boost::shared_ptr<T>(nt);
        } else {
            return boost::shared_ptr<T>();
        }
    }
    static boost::shared_ptr<T> clone(const std::shared_ptr<T> &t) {
        if (T *nt = clone_traits<T>::clone(t.get())) {
            return boost::shared_ptr<T>(nt);
        } else {
            return boost::shared_ptr<T>();
        }
    }
    static const unsigned int sharedPointerId = 1;
};

/**
  @internal
  Payload type trait specialization for QSharedPointer
  for documentation of the various members, see above
*/
template <typename T> struct PayloadTrait<QSharedPointer<T> >
{
    typedef T ElementType;
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T *>();
    }
    typedef typename Akonadi::SuperClass<T>::Type SuperElementType;
    typedef QSharedPointer<ElementType> Type;
    typedef QSharedPointer<SuperElementType> SuperType;
    static const bool isPolymorphic = !std::is_same<ElementType, SuperElementType>::value;
    static inline bool isNull(const Type &p)
    {
        return p.isNull();
    }
    template <typename U> static inline Type castFrom(const QSharedPointer<U> &p)
    {
        const Type sp = qSharedPointerDynamicCast<T, U>(p);
        if (!sp.isNull() || p.isNull()) {
            return sp;
        }
        throw PayloadException("qSharedPointerDynamicCast failed");
    }
    template <typename U> static inline bool canCastFrom(const QSharedPointer<U> &p)
    {
        const Type sp = qSharedPointerDynamicCast<T, U>(p);
        return !sp.isNull() || p.isNull();
    }
    template <typename U> static inline QSharedPointer<U> castTo(const Type &p)
    {
        const QSharedPointer<U> sp = qSharedPointerDynamicCast<U, T>(p);
        return sp;
    }
    static QSharedPointer<T> clone(const boost::shared_ptr<T> &t) {
        if (T *nt = clone_traits<T>::clone(t.get())) {
            return QSharedPointer<T>(nt);
        } else {
            return QSharedPointer<T>();
        }
    }
    static QSharedPointer<T> clone(const std::shared_ptr<T> &t) {
        if (T *nt = clone_traits<T>::clone(t.get())) {
            return QSharedPointer<T>(nt);
        } else {
            return QSharedPointer<T>();
        }
    }
    static const unsigned int sharedPointerId = 2;
};


/**
  @internal
  Payload type trait specialization for std::shared_ptr
  for documentation of the various members, see above
*/
template <typename T> struct PayloadTrait<std::shared_ptr<T> >
{
    typedef T ElementType;
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T *>();
    }
    typedef typename Akonadi::SuperClass<T>::Type SuperElementType;
    typedef std::shared_ptr<ElementType> Type;
    typedef std::shared_ptr<SuperElementType> SuperType;
    static const bool isPolymorphic = !std::is_same<ElementType, SuperElementType>::value;
    static inline bool isNull(const Type &p)
    {
        return p.get() == 0;
    }
    template <typename U> static inline Type castFrom(const std::shared_ptr<U> &p)
    {
        const Type sp = std::dynamic_pointer_cast<T, U>(p);
        if (sp.get() != 0 || p.get() == 0) {
            return sp;
        }
        throw PayloadException("std::dynamic_pointer_cast failed");
    }
    template <typename U> static inline bool canCastFrom(const std::shared_ptr<U> &p)
    {
        const Type sp = std::dynamic_pointer_cast<T, U>(p);
        return sp.get() != 0 || p.get() == 0;
    }
    template <typename U> static inline std::shared_ptr<U> castTo(const Type &p)
    {
        const std::shared_ptr<U> sp = std::dynamic_pointer_cast<U, T>(p);
        return sp;
    }
    static std::shared_ptr<T> clone(const boost::shared_ptr<T> &t) {
        if (T *nt = clone_traits<T>::clone(t.get())) {
            return std::shared_ptr<T>(nt);
        } else {
            return std::shared_ptr<T>();
        }
    }
    static std::shared_ptr<T> clone(const QSharedPointer<T> &t) {
        if (T *nt = clone_traits<T>::clone(t.data())) {
            return std::shared_ptr<T>(nt);
        } else {
            return std::shared_ptr<T>();
        }
    }
    static const unsigned int sharedPointerId = 3;
};


}

/**
 * @internal
 * Non-template base class for the payload container.
 * @note Move to Internal namespace for KDE5
 */
struct PayloadBase
{
    virtual ~PayloadBase()
    {
    }
    virtual PayloadBase *clone() const = 0;
    virtual const char *typeName() const = 0;
};

/**
 * @internal
 * Container for the actual payload object.
 * @note Move to Internal namespace for KDE5
 */
template <typename T>
struct Payload : public PayloadBase
{
    Payload()
    {
    }
    Payload(const T &p)
        : payload(p)
    {
    }

    PayloadBase *clone() const Q_DECL_OVERRIDE
    {
        return new Payload<T>(const_cast<Payload<T> *>(this)->payload);
    }

    const char *typeName() const Q_DECL_OVERRIDE
    {
        return typeid (const_cast<Payload<T> *>(this)).name();
    }

    T payload;
};

/**
 * @internal
 * abstract, will therefore always fail to compile for pointer payloads
 */
template <typename T>
struct Payload<T *> : public PayloadBase
{
};

namespace Internal {

/**
  @internal
  Basically a dynamic_cast that also works across DSO boundaries.
*/
template <typename T> inline Payload<T> *payload_cast(PayloadBase *payloadBase)
{
    Payload<T> *p = dynamic_cast<Payload<T> *>(payloadBase);
    // try harder to cast, workaround for some gcc issue with template instances in multiple DSO's
    if (!p && payloadBase && strcmp(payloadBase->typeName(), typeid (p).name()) == 0) {
        p = static_cast<Payload<T>*>(payloadBase);
    }
    return p;
}

}

}
//@endcond

#endif
