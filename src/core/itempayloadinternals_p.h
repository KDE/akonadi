/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "supertrait.h"

#include <QSharedPointer>

#include <memory>
#include <type_traits>
#include <typeinfo>

#include "exceptionbase.h"

/// @cond PRIVATE Doxygen 1.7.1 hangs processing this file. so skip it.
// for more info, see https://bugzilla.gnome.org/show_bug.cgi?id=531637

/* WARNING
 * The below is an implementation detail of the Item class. It is not to be
 * considered public API, and subject to change without notice
 */

namespace Akonadi
{
namespace Internal
{
template<typename T> struct has_clone_method {
private:
    template<typename S, S *(S::*)() const> struct sfinae {
    };
    struct No {
    };
    struct Yes {
        No no[2];
    };
    template<typename S> static No test(...);
    template<typename S> static Yes test(sfinae<S, &S::clone> *);

public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(Yes);
};

template<typename T, bool b> struct clone_traits_helper {
    // runtime error (commented in) or compile time error (commented out)?
    // ### runtime error, until we check has_clone_method in the
    // ### Item::payload<T> impl directly...
    template<typename U> static T *clone(U)
    {
        return nullptr;
    }
};

template<typename T> struct clone_traits_helper<T, true> {
    static T *clone(T *t)
    {
        return t ? t->clone() : nullptr;
    }
};

template<typename T> struct clone_traits : clone_traits_helper<T, has_clone_method<T>::value> {
};

template<typename T> struct shared_pointer_traits {
    static const bool defined = false;
};

template<typename T> struct shared_pointer_traits<QSharedPointer<T>> {
    static const bool defined = true;
    using element_type = T;

    template<typename S> struct make {
        using type = QSharedPointer<S>;
    };

    using next_shared_ptr = std::shared_ptr<T>;
};

template<typename T> struct shared_pointer_traits<std::shared_ptr<T>> {
    static const bool defined = true;
    using element_type = T;

    template<typename S> struct make {
        using type = std::shared_ptr<S>;
    };

    using next_shared_ptr = QSharedPointer<T>;
};

template<typename T> struct is_shared_pointer {
    static const bool value = shared_pointer_traits<T>::defined;
};

template<typename T> struct identity {
    using type = T;
};

template<typename T> struct get_hierarchy_root;

template<typename T, typename S> struct get_hierarchy_root_recurse : get_hierarchy_root<S> {
};

template<typename T> struct get_hierarchy_root_recurse<T, T> : identity<T> {
};

template<typename T> struct get_hierarchy_root : get_hierarchy_root_recurse<T, typename Akonadi::SuperClass<T>::Type> {
};

template<typename T> struct get_hierarchy_root<QSharedPointer<T>> {
    using type = QSharedPointer<typename get_hierarchy_root<T>::type>;
};

template<typename T> struct get_hierarchy_root<std::shared_ptr<T>> {
    using type = std::shared_ptr<typename get_hierarchy_root<T>::type>;
};

/**
  @internal
  Payload type traits. Implements specialized handling for polymorphic types and smart pointers.
  The default one is never used (as isPolymorphic is always false) and only contains safe dummy
  implementations to make the compiler happy (in practice it will always optimized away anyway).
*/
template<typename T> struct PayloadTrait {
    /// type of the payload object contained inside a shared pointer
    using ElementType = T;
    // the metatype id for the element type, or for pointer-to-element
    // type, if in a shared pointer
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T>();
    }
    /// type of the base class of the payload object inside a shared pointer,
    /// same as ElementType if there is no super class
    using SuperElementType = typename Akonadi::SuperClass<T>::Type;
    /// type of this payload object
    using Type = T;
    /// type of the payload to store a base class of this payload
    /// (eg. a shared pointer containing a pointer to SuperElementType)
    /// same as Type if there is not super class
    using SuperType = typename Akonadi::SuperClass<T>::Type;
    /// indicates if this payload is polymorphic, that it is a shared pointer
    /// and has a known super class
    static const bool isPolymorphic = false;
    /// checks an object of this payload type for being @c null
    static inline bool isNull(const Type &p)
    {
        Q_UNUSED(p)
        return true;
    }
    /// casts to Type from @c U
    /// throws a PayloadException if casting failed
    template<typename U> static inline Type castFrom(const U &)
    {
        throw PayloadException("you should never get here");
    }
    /// tests if casting from @c U to Type is possible
    template<typename U> static inline bool canCastFrom(const U &)
    {
        return false;
    }
    /// cast to @c U from Type
    template<typename U> static inline U castTo(const Type &)
    {
        throw PayloadException("you should never get here");
    }
    template<typename U> static T clone(const U &)
    {
        throw PayloadException("clone: you should never get here");
    }
    /// defines the type of shared pointer used (0: none, > 0: std::shared_ptr, QSharedPointer, ...)
    static const unsigned int sharedPointerId = 0;
};

/**
  @internal
  Payload type trait specialization for QSharedPointer
  for documentation of the various members, see above
*/
template<typename T> struct PayloadTrait<QSharedPointer<T>> {
    using ElementType = T;
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T *>();
    }
    using SuperElementType = typename Akonadi::SuperClass<T>::Type;
    using Type = QSharedPointer<ElementType>;
    using SuperType = QSharedPointer<SuperElementType>;
    static const bool isPolymorphic = !std::is_same<ElementType, SuperElementType>::value;
    static inline bool isNull(const Type &p)
    {
        return p.isNull();
    }
    template<typename U> static inline Type castFrom(const QSharedPointer<U> &p)
    {
        const Type sp = qSharedPointerDynamicCast<T, U>(p);
        if (!sp.isNull() || p.isNull()) {
            return sp;
        }
        throw PayloadException("qSharedPointerDynamicCast failed");
    }
    template<typename U> static inline bool canCastFrom(const QSharedPointer<U> &p)
    {
        const Type sp = qSharedPointerDynamicCast<T, U>(p);
        return !sp.isNull() || p.isNull();
    }
    template<typename U> static inline QSharedPointer<U> castTo(const Type &p)
    {
        const QSharedPointer<U> sp = qSharedPointerDynamicCast<U, T>(p);
        return sp;
    }
    static QSharedPointer<T> clone(const std::shared_ptr<T> &t)
    {
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
template<typename T> struct PayloadTrait<std::shared_ptr<T>> {
    using ElementType = T;
    static int elementMetaTypeId()
    {
        return qMetaTypeId<T *>();
    }
    using SuperElementType = typename Akonadi::SuperClass<T>::Type;
    using Type = std::shared_ptr<ElementType>;
    using SuperType = std::shared_ptr<SuperElementType>;
    static const bool isPolymorphic = !std::is_same<ElementType, SuperElementType>::value;
    static inline bool isNull(const Type &p)
    {
        return p.get() == nullptr;
    }
    template<typename U> static inline Type castFrom(const std::shared_ptr<U> &p)
    {
        const Type sp = std::dynamic_pointer_cast<T, U>(p);
        if (sp.get() != nullptr || p.get() == nullptr) {
            return sp;
        }
        throw PayloadException("std::dynamic_pointer_cast failed");
    }
    template<typename U> static inline bool canCastFrom(const std::shared_ptr<U> &p)
    {
        const Type sp = std::dynamic_pointer_cast<T, U>(p);
        return sp.get() != nullptr || p.get() == nullptr;
    }
    template<typename U> static inline std::shared_ptr<U> castTo(const Type &p)
    {
        const std::shared_ptr<U> sp = std::dynamic_pointer_cast<U, T>(p);
        return sp;
    }
    static std::shared_ptr<T> clone(const QSharedPointer<T> &t)
    {
        if (T *nt = clone_traits<T>::clone(t.data())) {
            return std::shared_ptr<T>(nt);
        } else {
            return std::shared_ptr<T>();
        }
    }
    static const unsigned int sharedPointerId = 3;
};

/**
 * @internal
 * Non-template base class for the payload container.
 */
struct PayloadBase {
    virtual ~PayloadBase() = default;
    virtual PayloadBase *clone() const = 0;
    virtual const char *typeName() const = 0;

protected:
    PayloadBase() = default;

private:
    Q_DISABLE_COPY_MOVE(PayloadBase)
};

/**
 * @internal
 * Container for the actual payload object.
 */
template<typename T> struct Payload : public PayloadBase {
    Payload(const T &p)
        : payload(p)
    {
    }

    PayloadBase *clone() const override
    {
        return new Payload<T>(const_cast<Payload<T> *>(this)->payload);
    }

    const char *typeName() const override
    {
        return typeid(const_cast<Payload<T> *>(this)).name();
    }

    T payload;
};

/**
 * @internal
 * abstract, will therefore always fail to compile for pointer payloads
 */
template<typename T> struct Payload<T *> : public PayloadBase {
};

/**
  @internal
  Basically a dynamic_cast that also works across DSO boundaries.
*/
template<typename T> inline Payload<T> *payload_cast(PayloadBase *payloadBase)
{
    auto p = dynamic_cast<Payload<T> *>(payloadBase);
    // try harder to cast, workaround for some gcc issue with template instances in multiple DSO's
    if (!p && payloadBase && strcmp(payloadBase->typeName(), typeid(p).name()) == 0) {
        p = static_cast<Payload<T> *>(payloadBase);
    }
    return p;
}

} // namespace Internal

} // namespace Akonadi

/// @endcond

