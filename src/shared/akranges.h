/*
    Copyright (C) 2018  Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_AKRANGES_H
#define AKONADI_AKRANGES_H

#include <QVector>
#include <QSet>

#include <algorithm>
#include <functional>

namespace Akonadi {

namespace detail {

struct ToQVector
{
    template<typename T> using Container = QVector<T>;
};

struct ToQSet
{
    template<typename T> using Container = QSet<T>;
};

struct ToQList
{
    template<typename T> using Container = QList<T>;
};

template<typename InContainer, typename OutContainer>
OutContainer copyContainer(const InContainer &in, std::true_type)
{
    OutContainer rv;
    rv.reserve(in.size());
    std::copy(std::begin(in), std::end(in), std::back_inserter(rv));
    return rv;
}

template<typename InContainer, typename OutContainer>
OutContainer copyContainer(const InContainer &in, std::false_type)
{
    OutContainer rv;
    for (const auto &v : in) {
        rv.insert(v); // can't use std::inserter on QSet, sadly :/
    }
    return rv;
}

template <typename...>
using void_type = void;

template <typename, template <typename> class, typename = void_type<>>
struct has_method: std::false_type
{};

template <typename T, template <typename> class Op>
struct has_method<T, Op, void_type<Op<T>>>: std::true_type
{};

template <typename T>
using push_back = decltype(std::declval<T>().push_back({}));

} // namespace detail

} // namespace Akonadi

// Magic to pipe container with a toQFoo object as a conversion
template<typename InContainer, typename OutFn,
    typename OutContainer = typename OutFn::template Container<typename InContainer::value_type>>
auto operator|(const InContainer &in, const OutFn &) -> OutContainer
{
    static_assert(std::is_same<typename InContainer::value_type,
                               typename OutContainer::value_type>::value,
            "We can only convert container types, not the value types.");
    static_assert(!std::is_same<InContainer, OutContainer>::value,
            "Wait, are you trying to convert a container to the same type?");

    return Akonadi::detail::copyContainer<InContainer, OutContainer>(
            in, Akonadi::detail::has_method<OutContainer, Akonadi::detail::push_back>{});
}


namespace Akonadi {

static constexpr auto toQVector = detail::ToQVector{};
static constexpr auto toQSet = detail::ToQSet{};
static constexpr auto toQList = detail::ToQList{};

} // namespace Akonadi


#endif
