/*
    Copyright (C) 2018 - 2019  Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_AKHELPERS_H_
#define AKONADI_AKHELPERS_H_

#include <type_traits>
#include <utility>

namespace Akonadi
{
namespace detail
{

// C++14-compatible implementation of std::invoke(), based on implementation
// from https://en.cppreference.com/w/cpp/utility/functional/invoke

template<typename T>
struct is_reference_wrapper : std::false_type {};
template<typename T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};
template<typename T>
constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

template<typename T>
constexpr bool is_member_function_pointer_v = std::is_member_function_pointer<std::remove_cv_t<std::remove_reference_t<T>>>::value;

template<typename T, typename Type, typename T1, typename ... Args>
auto invoke(Type T::* fun, T1 &&t1, Args && ... args) ->
    std::enable_if_t<is_member_function_pointer_v<decltype(fun)>
                     && std::is_base_of<T, std::decay_t<T1>>::value,
                     std::result_of_t<decltype(fun)(T, Args ...)>>
{
    return (std::forward<T1>(t1).*fun)(std::forward<Args>(args) ...);
}

template<typename T, typename Type, typename T1, typename ... Args>
auto invoke(Type T::* fun, T1 &&t1, Args && ... args) ->
    std::enable_if_t<is_member_function_pointer_v<decltype(fun)>
                     && is_reference_wrapper_v<std::decay_t<T1>>,
                     std::result_of_t<decltype(fun)(T, Args ...)>>
{
    return (t1.get().*fun)(std::forward<Args>(args) ...);
}

template<typename T, typename Type, typename T1, typename ... Args>
auto invoke(Type T::* fun, T1 &&t1, Args && ... args) ->
    std::enable_if_t<is_member_function_pointer_v<decltype(fun)>
                     && !std::is_base_of<T, std::decay_t<T1>>::value
                     && !is_reference_wrapper_v<std::decay_t<T1>>,
                     std::result_of_t<decltype(fun)(T1, Args ...)>>
{
    return ((*std::forward<T1>(t1)).*fun)(std::forward<Args>(args) ...);
}

template<typename T, typename Type, typename T1, typename ... Args>
auto invoke(Type T::* fun, T1 &&t1, Args && ... args) ->
    std::enable_if_t<!is_member_function_pointer_v<decltype(fun)>
                     && std::is_base_of<T1, std::decay_t<T1>>::value,
                     std::result_of_t<decltype(fun)(T1, Args ...)>>
{
    return std::forward<T1>(t1).*fun;
}

template<typename T, typename Type, typename T1, typename ... Args>
auto invoke(Type T::* fun, T1 &&t1, Args && ... args) ->
    std::enable_if_t<!is_member_function_pointer_v<decltype(fun)>
                     && is_reference_wrapper_v<std::decay_t<T1>>,
                     std::result_of_t<decltype(fun)(T1, Args ...)>>
{
    return t1.get().*fun;
}

template<typename T, typename Type, typename T1, typename ... Args>
auto invoke(Type T::* fun, T1 &&t1, Args && ... args) ->
    std::enable_if_t<!is_member_function_pointer_v<decltype(fun)>
                     && !std::is_base_of<T1, std::decay_t<T1>>::value
                     && !is_reference_wrapper_v<std::decay_t<T1>>,
                     std::result_of_t<decltype(fun)(T1, Args ...)>>
{
    return (*std::forward<T1>(t1)).*fun;
}

template<typename Fun, typename ... Args>
auto invoke(Fun &&fun, Args && ... args)
{
    return std::forward<Fun>(fun)(std::forward<Args>(args) ...);
}

} // namespace detail

template<typename Fun, typename ... Args>
auto invoke(Fun &&fun, Args && ... args)
{
    return detail::invoke(std::forward<Fun>(fun), std::forward<Args>(args) ...);
}

static const auto IsNull = [](auto ptr) { return !(bool)ptr; };
static const auto IsNotNull = [](auto ptr) { return (bool)ptr; };

} // namespace Akonadi

#endif
