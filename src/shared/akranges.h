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

#ifndef QT_STRICT_ITERATORS
// Without strict iterator QVector<T>::iterator is just a typedef to T*,
// which breaks some of the template magic below. QT_STRICT_ITERATORS
// are a good thing anyway...
#error AkRanges requires QT_STRICT_ITERATORS to be enabled.
#endif

#include <QVector>
#include <QSet>
#include <QDebug>
#include <algorithm>
#include <functional>

namespace Akonadi {

namespace detail {

template<template<typename> class Cont>
struct To_
{
    template<typename T> using Container = Cont<T>;
};

template<typename InContainer, typename OutContainer>
OutContainer copyContainer(const InContainer &in, std::true_type)
{
    OutContainer rv;
    rv.reserve(in.size());
    std::move(std::begin(in), std::end(in), std::back_inserter(rv));
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

template<typename Iterator, typename TransformFn = void*>
struct LazyIterator : public std::iterator<
                        typename Iterator::iterator_category,
                        typename Iterator::value_type,
                        typename Iterator::difference_type,
                        typename Iterator::pointer,
                        typename Iterator::reference>
{
public:
    LazyIterator(const Iterator &iter): mIter(iter) {};
    LazyIterator(const Iterator &iter, const TransformFn &fn)
        : mIter(iter), mFn(fn) {}

    LazyIterator<Iterator, TransformFn> &operator++()
    {
        mIter++;
        return *this;
    }
    LazyIterator<Iterator, TransformFn> operator++(int)
    {
        auto ret = *this;
        ++(*this);
        return ret;
    };

    bool operator==(const LazyIterator<Iterator, TransformFn> &other) const
    {
        return mIter == other.mIter;
    }
    bool operator!=(const LazyIterator<Iterator, TransformFn> &other) const
    {
        return mIter != other.mIter;
    }

    auto operator*() const
    {
        return std::move(getValue<TransformFn>(mIter));
   }

    auto operator-(const LazyIterator<Iterator, TransformFn> &other) const
    {
        return mIter - other.mIter;
    }

    const Iterator &iter() const
    {
        return mIter;
    }
private:
    /*
    template<typename T>
    typename std::enable_if<std::is_same<T, void*>::value, typename Iterator::value_type>::type
    getValue(const Iterator &iter) const
    {
        return *iter;
    }
    */
    template<typename T>
    typename std::enable_if<!std::is_same<T, void*>::value, typename Iterator::value_type>::type
    getValue(const Iterator &iter, std::true_type = {}) const
    {
        return mFn(*iter);
    }

    Iterator mIter;
    TransformFn mFn = {};
};

template<typename Iterator, typename TransformFn = void*>
struct Range
{
public:
    using iterator = Iterator;
    using value_type = typename Iterator::value_type;

    Range(Iterator &&begin, Iterator &&end)
        : mBegin(begin), mEnd(end) {}
    Range(Iterator &&begin, Iterator &&end, const TransformFn &fn)
        : mBegin(begin, fn), mEnd(end, fn) {}

    LazyIterator<Iterator, TransformFn> begin() const
    {
        return mBegin;
    }
    LazyIterator<Iterator, TransformFn> end() const
    {
        return mEnd;
    }

    auto size() const
    {
        return mEnd.iter() - mBegin.iter();
    }

private:
    LazyIterator<Iterator, TransformFn> mBegin;
    LazyIterator<Iterator, TransformFn> mEnd;
};


template<typename T>
using IsRange = typename std::is_same<T, Range<typename T::iterator>>;

template<typename TransformFn>
struct Transform_
{
    using Fn = TransformFn;

    Transform_(TransformFn &&fn): mFn(std::forward<TransformFn>(fn)) {}
    TransformFn &&mFn;
};

} // namespace detail

} // namespace Akonadi


template<typename InRange, template<typename> class OutContainer,
         typename T = typename InRange::value_type>
typename std::enable_if<Akonadi::detail::IsRange<InRange>::value, OutContainer<T>>::type
operator|(const InRange &in, const Akonadi::detail::To_<OutContainer> &)
{
    using namespace Akonadi::detail;
    return copyContainer<InRange, OutContainer<T>>(
            in, has_method<OutContainer<T>, push_back>{});
}

// Magic to pipe container with a toQFoo object as a conversion
template<typename InContainer, template<typename> class OutContainer,
         typename T = typename InContainer::value_type>
typename std::enable_if<!Akonadi::detail::IsRange<InContainer>::value, OutContainer<T>>::type
operator|(const InContainer &in, const Akonadi::detail::To_<OutContainer> &)
{
    static_assert(!std::is_same<InContainer, OutContainer<T>>::value,
            "Wait, are you trying to convert a container to the same type?");

    using namespace Akonadi::detail;
    return copyContainer<InContainer, OutContainer<T>>(
            in, has_method<OutContainer<T>, push_back>{});
}

template<typename InContainer, typename TransformFn,
         typename It = typename InContainer::const_iterator>
auto operator|(const InContainer &in, const Akonadi::detail::Transform_<TransformFn> &t)
{
    using namespace Akonadi::detail;
    return Range<It, TransformFn>(in.cbegin(), in.cend(), *reinterpret_cast<const TransformFn*>(&t));
}

namespace Akonadi {

static constexpr auto toQVector = detail::To_<QVector>{};
static constexpr auto toQSet = detail::To_<QSet>{};
static constexpr auto toQList = detail::To_<QList>{};

template<typename TransformFn>
detail::Transform_<TransformFn> transform(TransformFn &&fn)
{
    return detail::Transform_<TransformFn>(std::forward<TransformFn>(fn));
}

} // namespace Akonadi

#endif
