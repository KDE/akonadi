/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <type_traits>
#include <utility>

namespace AkTraits
{
namespace detail
{
template<typename...>
struct conjunction : std::true_type {
};
template<typename T>
struct conjunction<T> : T {
};
template<typename T, typename... Ts>
struct conjunction<T, Ts...> : std::conditional_t<bool(T::value), conjunction<Ts...>, T> {
};

#define DECLARE_HAS_MEBER_TYPE(type_name)                                                                                                                      \
    template<typename T, typename U = std::void_t<>>                                                                                                           \
    struct hasMember_##type_name {                                                                                                                             \
        static constexpr bool value = false;                                                                                                                   \
    };                                                                                                                                                         \
                                                                                                                                                               \
    template<typename T>                                                                                                                                       \
    struct hasMember_##type_name<T, std::void_t<typename T::type_name>> : std::true_type {                                                                     \
    };

DECLARE_HAS_MEBER_TYPE(value_type)

/// TODO: Use Boost TTI instead?
#define DECLARE_HAS_METHOD_GENERIC_IMPL(name, fun, sign)                                                                                                       \
    template<typename T, typename F = sign>                                                                                                                    \
    struct hasMethod_##name {                                                                                                                                  \
    public:                                                                                                                                                    \
        template<typename UType, UType>                                                                                                                        \
        struct helperClass;                                                                                                                                    \
                                                                                                                                                               \
        using True = char;                                                                                                                                     \
        using False = struct {                                                                                                                                 \
            char dummy_[2];                                                                                                                                    \
        };                                                                                                                                                     \
                                                                                                                                                               \
        template<typename U>                                                                                                                                   \
        static True helper(helperClass<F, &U::fun> *);                                                                                                         \
        template<typename>                                                                                                                                     \
        static False helper(...);                                                                                                                              \
                                                                                                                                                               \
    public:                                                                                                                                                    \
        static constexpr bool value = sizeof(helper<T>(nullptr)) == sizeof(True);                                                                              \
    };

#define DECLARE_HAS_METHOD_GENERIC_CONST(fun, R, ...) DECLARE_HAS_METHOD_GENERIC_IMPL(fun##_const, fun, R (T::*)(__VA_ARGS__) const)

#define DECLARE_HAS_METHOD_GENERIC(fun, R, ...) DECLARE_HAS_METHOD_GENERIC_IMPL(fun, fun, R (T::*)(__VA_ARGS__))

// deal with Qt6 interface changes in QList::push_back/QList::insert
template<typename T, typename = std::void_t<>>
struct parameter_type {
    typedef const typename T::value_type &type;
};
template<typename T>
struct parameter_type<T, std::void_t<typename T::parameter_type>> {
    typedef typename T::parameter_type type;
};

DECLARE_HAS_METHOD_GENERIC_CONST(size, qsizetype, void)
DECLARE_HAS_METHOD_GENERIC(push_back, void, typename parameter_type<T>::type)
DECLARE_HAS_METHOD_GENERIC(insert, typename T::iterator, typename parameter_type<T>::type)
DECLARE_HAS_METHOD_GENERIC(reserve, void, qsizetype)

#define DECLARE_HAS_FUNCTION(name, fun)                                                                                                                        \
    template<typename T>                                                                                                                                       \
    struct has_##name {                                                                                                                                        \
        template<typename U>                                                                                                                                   \
        struct helperClass;                                                                                                                                    \
                                                                                                                                                               \
        using True = char;                                                                                                                                     \
        using False = struct {                                                                                                                                 \
            char dummy_[2];                                                                                                                                    \
        };                                                                                                                                                     \
                                                                                                                                                               \
        template<typename U>                                                                                                                                   \
        static True helper(helperClass<decltype(fun(std::declval<T>()))> *);                                                                                   \
        template<typename>                                                                                                                                     \
        static False helper(...);                                                                                                                              \
                                                                                                                                                               \
    public:                                                                                                                                                    \
        static constexpr bool value = sizeof(helper<T>(nullptr)) == sizeof(True);                                                                              \
    };

// For some obscure reason QVector::begin() actually has a default
// argument, but QList::begin() does not, thus a regular hasMethod_* check
// won't cut it here. Instead we check whether the container object can be
// used with std::begin() and std::end() helpers.
// Check for constness can be performed by passing "const T" to the type.
DECLARE_HAS_FUNCTION(begin, std::begin)
DECLARE_HAS_FUNCTION(end, std::end)

/// This is a very incomplete set of Container named requirement, but I'm
/// too lazy to implement all of them, but this should be good enough to match
/// regular Qt containers and /not/ match arbitrary non-container types
template<typename T>
struct isContainer
    : conjunction<std::is_constructible<T>, hasMember_value_type<T>, has_begin<T>, has_begin<const T>, has_end<T>, has_end<const T>, hasMethod_size_const<T>> {
};

/// Matches anything that is a container and has push_back() method.
template<typename T>
struct isAppendable : conjunction<isContainer<T>, hasMethod_push_back<T>> {
};

/// Matches anything that is a container and has insert() method.
template<typename T>
struct isInsertable : conjunction<isContainer<T>, hasMethod_insert<T>> {
};

/// Matches anything that is a container and has reserve() method.
template<typename T>
struct isReservable : conjunction<isContainer<T>, hasMethod_reserve<T>> {
};
}

template<typename T>
constexpr bool isAppendable = detail::isAppendable<T>::value;

template<typename T>
constexpr bool isInsertable = detail::isInsertable<T>::value;

template<typename T>
constexpr bool isReservable = detail::isReservable<T>::value;

} // namespace AkTraits

#define AK_PP_CAT_(X, Y) X##Y
#define AK_PP_CAT(X, Y) AK_PP_CAT_(X, Y)

#define AK_REQUIRES(...) bool AK_PP_CAT(_ak_requires_, __LINE__) = false, std::enable_if_t < AK_PP_CAT(_ak_requires_, __LINE__) || (__VA_ARGS__) > * = nullptr
