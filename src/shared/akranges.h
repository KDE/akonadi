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

#ifndef AKONADI_AKRANGES_H
#define AKONADI_AKRANGES_H

#ifndef QT_STRICT_ITERATORS
// Without strict iterator QVector<T>::iterator is just a typedef to T*,
// which breaks some of the template magic below. QT_STRICT_ITERATORS
// are a good thing anyway...
#error AkRanges requires QT_STRICT_ITERATORS to be enabled.
#endif

#include "aktraits.h"
#include "akhelpers.h"

#include <QVector>
#include <QSet>
#include <QDebug>

#include <algorithm>
#include <functional>
#include <utility>
#include <type_traits>

namespace Akonadi {

namespace detail {

template<template<typename> class Cont>
struct To_
{
    template<typename T> using Container = Cont<T>;
};

template<typename InContainer,
         typename OutContainer,
         AK_REQUIRES(AkTraits::isAppendable<OutContainer> && AkTraits::isReservable<OutContainer>)
        >
OutContainer copyContainer(const InContainer &in)
{
    OutContainer rv;
    rv.reserve(in.size());
    for (auto &&v : in) {
        rv.push_back(std::move(v));
    }
    return rv;
}

template<typename InContainer,
         typename OutContainer,
         AK_REQUIRES(AkTraits::isInsertable<OutContainer>)
        >
OutContainer copyContainer(const InContainer &in)
{
    OutContainer rv;
    for (const auto &v : in) {
        rv.insert(v);
    }
    return rv;
}

template<typename IterImpl,
         typename Container,
         typename Iterator = typename Container::const_iterator
        >
struct IteratorBase
{
public:
    using iterator_category = typename Iterator::iterator_category;
    using value_type = typename Iterator::value_type;
    using difference_type = typename Iterator::difference_type;
    using pointer = typename Iterator::pointer;
    using reference = typename Iterator::reference;

    IteratorBase(const IteratorBase<IterImpl, Container> &other)
        : mIter(other.mIter), mContainer(other.mContainer)
    {}

    IterImpl &operator++()
    {
        ++static_cast<IterImpl*>(this)->mIter;
        return *static_cast<IterImpl*>(this);
    }

    IterImpl operator++(int)
    {
        auto ret = *static_cast<IterImpl*>(this);
        ++static_cast<IterImpl*>(this)->mIter;
        return ret;
    }

    bool operator==(const IterImpl &other) const
    {
        return mIter == other.mIter;
    }

    bool operator!=(const IterImpl &other) const
    {
        return !(*static_cast<const IterImpl*>(this) == other);
    }

    bool operator<(const IterImpl &other) const
    {
        return mIter < other.mIter;
    }

    auto operator-(const IterImpl &other) const
    {
        return mIter - other.mIter;
    }

    auto operator*() const
    {
        return *mIter;
    }

protected:
    IteratorBase(const Iterator &iter, const Container &container)
        : mIter(iter), mContainer(container)
    {}
    IteratorBase(const Iterator &iter, Container &&container)
        : mIter(iter), mContainer(std::move(container))
    {}

    Iterator mIter;
    Container mContainer;
};

template<typename Container,
         typename TransformFn,
         typename Iterator = typename Container::const_iterator
        >
struct TransformIterator : public IteratorBase<TransformIterator<Container, TransformFn>, Container>
{
private:
    template<typename ... T>
    struct ResultOf;

    template<typename R, typename ... Args>
    struct ResultOf<R(Args ...)> {
        using type = R;
    };

    template<typename ... Ts>
    using FuncHelper = decltype(Akonadi::invoke(std::declval<Ts>() ...))(Ts ...);
    using IteratorValueType = typename ResultOf<FuncHelper<TransformFn, typename Iterator::value_type>>::type;
public:
    using value_type = IteratorValueType;
    using pointer = IteratorValueType *;         // FIXME: preserve const-ness
    using reference = const IteratorValueType &; // FIXME: preserve const-ness

    TransformIterator(const Iterator &iter, const TransformFn &fn, const Container &container)
        : IteratorBase<TransformIterator<Container, TransformFn>, Container>(iter, container)
        , mFn(fn)
    {
    }

    auto operator*() const
    {
        return Akonadi::invoke(mFn, *this->mIter);
    }

private:
    TransformFn mFn;
};

template<typename Container,
         typename Predicate,
         typename Iterator = typename Container::const_iterator
        >
class FilterIterator : public IteratorBase<FilterIterator<Container, Predicate>, Container>
{
public:
    FilterIterator(const Iterator &iter, const Iterator &end,
                   const Predicate &predicate,
                   const Container &container)
        : IteratorBase<FilterIterator<Container, Predicate>, Container>(iter, container)
        , mPredicate(predicate), mEnd(end)
    {
        while (this->mIter != mEnd && !Akonadi::invoke(mPredicate, *this->mIter)) {
            ++this->mIter;
        }
    }

    auto &operator++()
    {
        if (this->mIter != mEnd) {
            do {
                ++this->mIter;
            } while (this->mIter != mEnd && !Akonadi::invoke(mPredicate, *this->mIter));
        }
        return *this;
    }

    auto operator++(int)
    {
        auto it = *this;
        ++(*this);
        return it;
    }

private:
    Predicate mPredicate;
    Iterator mEnd;
};


template<typename Iterator>
struct Range
{
public:
    using iterator = Iterator;
    using const_iterator = Iterator;
    using value_type = typename Iterator::value_type;

    Range(Iterator &&begin, Iterator &&end)
        : mBegin(std::move(begin))
        , mEnd(std::move(end))
    {}

    Iterator begin() const
    {
        return mBegin;
    }

    Iterator cbegin() const
    {
        return mBegin;
    }

    Iterator end() const
    {
        return mEnd;
    }

    Iterator cend() const
    {
        return mEnd;
    }

    auto size() const
    {
        return std::distance(mBegin, mEnd);
    }

private:
    Iterator mBegin;
    Iterator mEnd;
};

template<typename T>
using IsRange = typename std::is_same<T, Range<typename T::iterator>>;

template<typename TransformFn>
struct Transform_
{
    TransformFn mFn;
};

template<typename PredicateFn>
struct Filter_
{
    PredicateFn mFn;
};

template<typename EachFun>
struct ForEach_
{
    EachFun mFn;
};

} // namespace detail
} // namespace Akonadi

// Generic operator| for To_<> convertor
template<typename InContainer,
         template<typename> class OutContainer,
         typename T = typename InContainer::value_type
        >
auto operator|(const InContainer &in,
               const Akonadi::detail::To_<OutContainer> &) -> OutContainer<T>
{
    using namespace Akonadi::detail;
    return copyContainer<InContainer, OutContainer<T>>(in);
}

// Specialization for case when InContainer and OutContainer are identical
// Create a copy, but for Qt container this is very cheap due to implicit sharing.
template<template<typename> class InContainer,
         typename T
        >
auto operator|(const InContainer<T> &in,
               const Akonadi::detail::To_<InContainer> &) -> InContainer<T>
{
    return in;
}



// Generic operator| for transform()
template<typename InContainer,
         typename TransformFn
        >
auto operator|(const InContainer &in,
               const Akonadi::detail::Transform_<TransformFn> &t)
{
    using namespace Akonadi::detail;
    using OutIt = TransformIterator<InContainer, TransformFn>;
    return Range<OutIt>(OutIt(std::cbegin(in), t.mFn, in), OutIt(std::cend(in), t.mFn, in));
}


// Generic operator| for filter()
template<typename InContainer,
         typename PredicateFn
        >
auto operator|(const InContainer &in,
               const Akonadi::detail::Filter_<PredicateFn> &p)
{
    using namespace Akonadi::detail;
    using OutIt = FilterIterator<InContainer, PredicateFn>;
    return Range<OutIt>(OutIt(std::cbegin(in), std::cend(in), p.mFn, in),
                        OutIt(std::cend(in), std::cend(in), p.mFn, in));
}


// Generic operator| fo foreach()
template<typename InContainer,
         typename EachFun
        >
auto operator|(const InContainer &in,
               Akonadi::detail::ForEach_<EachFun> fun)
{
    std::for_each(std::cbegin(in), std::cend(in),
                  [&fun](const auto &val) {
                      Akonadi::invoke(fun.mFn, val);
                  });
    return in;
}


namespace Akonadi {

/// Non-lazily convert given range or container to QVector
static constexpr auto toQVector = detail::To_<QVector>{};
/// Non-lazily convert given range or container to QSet
static constexpr auto toQSet = detail::To_<QSet>{};
/// Non-lazily convert given range or container to QList
static constexpr auto toQList = detail::To_<QList>{};

/// Lazily transform each element of a range or container using given transformation
template<typename TransformFn>
detail::Transform_<TransformFn> transform(TransformFn &&fn)
{
    return detail::Transform_<TransformFn>{std::forward<TransformFn>(fn)};
}

/// Lazily filters a range or container by applying given predicate on each element
template<typename PredicateFn>
detail::Filter_<PredicateFn> filter(PredicateFn &&fn)
{
    return detail::Filter_<PredicateFn>{std::forward<PredicateFn>(fn)};
}

/// Non-lazily call EachFun for each element of the container or range
template<typename EachFun>
detail::ForEach_<EachFun> forEach(EachFun &&fn)
{
    return detail::ForEach_<EachFun>{std::forward<EachFun>(fn)};
}

/// Create a range, a view on a container from the given pair fo iterators
template<typename Iterator1, typename Iterator2,
         typename It = std::remove_reference_t<Iterator1>
        >
detail::Range<It> range(Iterator1 begin, Iterator2 end)
{
    return detail::Range<It>(std::move(begin), std::move(end));
}



} // namespace Akonadi

#endif
