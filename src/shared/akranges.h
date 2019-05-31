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

#include "aktraits.h"
#include "akhelpers.h"

#include <QVector>
#include <QSet>
#include <QDebug>

#include <algorithm>
#include <functional>
#include <utility>
#include <type_traits>
#include <iterator>

namespace Akonadi {

namespace detail {

template<template<typename> class Cont>
struct To_
{
    template<typename T> using Container = Cont<T>;
};

struct Values_ {};
struct Keys_ {};

template<typename RangeLike, typename OutContainer,
         AK_REQUIRES(AkTraits::isAppendable<OutContainer> && AkTraits::isReservable<OutContainer>)>
OutContainer copyContainer(const RangeLike &range)
{
    OutContainer rv;
    rv.reserve(range.size());
    for (auto &&v : range) {
        rv.push_back(std::move(v));
    }
    return rv;
}

template<typename RangeLike, typename OutContainer,
         AK_REQUIRES(AkTraits::isInsertable<OutContainer>)>
OutContainer copyContainer(const RangeLike &range)
{
    OutContainer rv;
    for (const auto &v : range) {
        rv.insert(v);
    }
    return rv;
}

template<typename Iterator>
struct IteratorTrait {
    using iterator_category = typename Iterator::iterator_category;
    using value_type = typename Iterator::value_type;
    using difference_type = typename Iterator::difference_type;
    using pointer = typename Iterator::pointer;
    using reference = typename Iterator::reference;
};

// Without QT_STRICT_ITERATORS QVector and QList iterators do not satisfy STL
// iterator concepts since they are nothing more but typedefs to T* - for those
// we need to provide custom traits.
template<typename Iterator>
struct IteratorTrait<Iterator*> {
    // QTypedArrayData::iterator::iterator_category
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Iterator;
    using difference_type = int;
    using pointer = Iterator*;
    using reference = Iterator&;
};

template<typename Iterator>
struct IteratorTrait<const Iterator *> {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Iterator;
    using difference_type = int;
    using pointer = const Iterator *;
    using reference = const Iterator &;
};

template<typename IterImpl, typename RangeLike, typename Iterator = typename RangeLike::const_iterator>
struct IteratorBase
{
public:
    using iterator_category = typename IteratorTrait<Iterator>::iterator_category;
    using value_type = typename IteratorTrait<Iterator>::value_type;
    using difference_type = typename IteratorTrait<Iterator>::difference_type;
    using pointer = typename IteratorTrait<Iterator>::pointer;
    using reference = typename IteratorTrait<Iterator>::reference;

    IteratorBase(const IteratorBase<IterImpl, RangeLike> &other)
        : mIter(other.mIter), mRange(other.mRange)
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
    IteratorBase(const Iterator &iter, const RangeLike &range)
        : mIter(iter), mRange(range)
    {}
    IteratorBase(const Iterator &iter, RangeLike &&range)
        : mIter(iter), mRange(std::move(range))
    {}

    Iterator mIter;
    RangeLike mRange;
};

template<typename RangeLike, typename TransformFn, typename Iterator = typename RangeLike::const_iterator>
struct TransformIterator : public IteratorBase<TransformIterator<RangeLike, TransformFn>, RangeLike>
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
    using IteratorValueType = typename ResultOf<FuncHelper<TransformFn, typename IteratorTrait<Iterator>::value_type>>::type;
public:
    using value_type = IteratorValueType;
    using pointer = IteratorValueType *;         // FIXME: preserve const-ness
    using reference = const IteratorValueType &; // FIXME: preserve const-ness

    TransformIterator(const Iterator &iter, const TransformFn &fn, const RangeLike &range)
        : IteratorBase<TransformIterator<RangeLike, TransformFn>, RangeLike>(iter, range)
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

template<typename RangeLike, typename Predicate, typename Iterator = typename RangeLike::const_iterator>
class FilterIterator : public IteratorBase<FilterIterator<RangeLike, Predicate>, RangeLike>
{
public:
    FilterIterator(const Iterator &iter, const Iterator &end, const Predicate &predicate, const RangeLike &range)
        : IteratorBase<FilterIterator<RangeLike, Predicate>, RangeLike>(iter, range)
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


template<typename Container, int Pos, typename Iterator = typename Container::const_key_value_iterator>
class AssociativeContainerIterator
    : public IteratorBase<AssociativeContainerIterator<Container, Pos>, Container, Iterator>
{
public:
    using value_type = std::remove_const_t<std::remove_reference_t<
                            typename std::tuple_element<Pos, typename Iterator::value_type>::type>>;
    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference_t<value_type>;

    AssociativeContainerIterator(const Iterator &iter, const Container &container)
        : IteratorBase<AssociativeContainerIterator<Container, Pos>, Container, Iterator>(iter, container)
    {}

    auto operator*() const
    {
        return std::get<Pos>(*this->mIter);
    }
};

template<typename Container>
using AssociativeContainerKeyIterator = AssociativeContainerIterator<Container, 0>;
template<typename Container>
using AssociativeContainerValueIterator = AssociativeContainerIterator<Container, 1>;


template<typename Iterator>
struct Range
{
public:
    using iterator = Iterator;
    using const_iterator = Iterator;
    using value_type = typename detail::IteratorTrait<Iterator>::value_type;

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

template<typename Predicate>
struct All_
{
    Predicate mFn;
};

template<typename Predicate>
struct Any_
{
    Predicate mFn;
};

} // namespace detail
} // namespace Akonadi

// Generic operator| for To_<> convertor
template<typename RangeLike, template<typename> class OutContainer, typename T = typename RangeLike::value_type>
auto operator|(const RangeLike &range, const Akonadi::detail::To_<OutContainer> &) -> OutContainer<T>
{
    using namespace Akonadi::detail;
    return copyContainer<RangeLike, OutContainer<T>>(range);
}

// Specialization for case when InContainer and OutContainer are identical
// Create a copy, but for Qt container this is very cheap due to implicit sharing.
template<template<typename> class InContainer, typename T>
auto operator|(const InContainer<T> &in, const Akonadi::detail::To_<InContainer> &) -> InContainer<T>
{
    return in;
}

// Generic operator| for transform()
template<typename RangeLike, typename TransformFn>
auto operator|(const RangeLike &range, const Akonadi::detail::Transform_<TransformFn> &t)
{
    using namespace Akonadi::detail;
    using OutIt = TransformIterator<RangeLike, TransformFn>;
    return Range<OutIt>(OutIt(std::cbegin(range), t.mFn, range), OutIt(std::cend(range), t.mFn, range));
}

// Generic operator| for filter()
template<typename RangeLike, typename PredicateFn>
auto operator|(const RangeLike &range, const Akonadi::detail::Filter_<PredicateFn> &p)
{
    using namespace Akonadi::detail;
    using OutIt = FilterIterator<RangeLike, PredicateFn>;
    return Range<OutIt>(OutIt(std::cbegin(range), std::cend(range), p.mFn, range),
                        OutIt(std::cend(range), std::cend(range), p.mFn, range));
}

// Generic operator| fo foreach()
template<typename RangeLike, typename EachFun>
auto operator|(const RangeLike&range, Akonadi::detail::ForEach_<EachFun> fun)
{
    std::for_each(std::cbegin(range), std::cend(range),
                  [&fun](const auto &val) {
                      Akonadi::invoke(fun.mFn, val);
                  });
    return range;
}

// Generic operator| for all
template<typename RangeLike, typename PredicateFn>
auto operator|(const RangeLike &range, Akonadi::detail::All_<PredicateFn> fun)
{
    return std::all_of(std::cbegin(range), std::cend(range), fun.mFn);
}

// Generic operator| for any
template<typename RangeLike, typename PredicateFn>
auto operator|(const RangeLike &range, Akonadi::detail::Any_<PredicateFn> fun)
{
    return std::any_of(std::cbegin(range), std::cend(range), fun.mFn);
}

// Generic operator| for keys
template<typename Container>
auto operator|(const Container &in, Akonadi::detail::Keys_)
{
    using namespace Akonadi::detail;
    using OutIt = AssociativeContainerKeyIterator<Container>;
    return Range<OutIt>(OutIt(in.constKeyValueBegin(), in), OutIt(in.constKeyValueEnd(), in));
}


// Generic operator| for values
template<typename Container>
auto operator|(const Container &in, Akonadi::detail::Values_)
{
    using namespace Akonadi::detail;
    using OutIt = AssociativeContainerValueIterator<Container>;
    return Range<OutIt>(OutIt(in.constKeyValueBegin(), in), OutIt(in.constKeyValueEnd(), in));
}


namespace Akonadi {

/// Non-lazily convert given range or container to QVector
static constexpr auto toQVector = detail::To_<QVector>{};
/// Non-lazily convert given range or container to QSet
static constexpr auto toQSet = detail::To_<QSet>{};
/// Non-lazily convert given range or container to QList
static constexpr auto toQList = detail::To_<QList>{};
/// Lazily extract values from an associative container
static constexpr auto values = detail::Values_{};
/// Lazily extract keys from an associative container
static constexpr auto keys = detail::Keys_{};

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

/// Non-lazily check that all elements in the range satisfy given predicate
template<typename Predicate>
detail::All_<Predicate> all(Predicate &&fn)
{
    return detail::All_<Predicate>{std::forward<Predicate>(fn)};
}

/// Non-lazily check that at least one element in range satisfies the given predicate
template<typename Predicate>
detail::Any_<Predicate> any(Predicate &&fn)
{
    return detail::Any_<Predicate>{std::forward<Predicate>(fn)};
}

} // namespace Akonadi

#endif
