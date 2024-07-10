/*
    SPDX-FileCopyrightText: 2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <concepts>
#include <utility>

namespace AkTraits
{
/// This is a very incomplete set of Container named requirement, but I'm
/// too lazy to implement all of them, but this should be good enough to match
/// regular Qt containers and /not/ match arbitrary non-container types
template<typename T>
concept Container = requires(const T &cont) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;

    { std::begin(cont) } -> std::same_as<typename T::const_iterator>;
    { std::end(cont) } -> std::same_as<typename T::const_iterator>;
    { cont.size() } -> std::same_as<typename T::size_type>;
};

template<typename T>
concept AppendableContainer = Container<T> && requires(T cont) {
    { cont.push_back(std::declval<typename T::value_type>()) };
};

template<typename T>
concept InsertableContainer = Container<T> && requires(T cont) {
    { cont.insert(std::declval<typename T::iterator>(), std::declval<typename T::value_type>()) };
};

template<typename T>
concept ReservableContainer = Container<T> && requires(T cont) {
    { cont.reserve(std::declval<typename T::size_type>()) };
};

} // namespace AkTraits
