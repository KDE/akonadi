/*
    SPDX-FileCopyrightText: 2018-2019 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akhelpers.h"
#include "aktraits.h"

#include <QList>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace AkRanges
{
namespace detail
{
template<typename RangeLike, typename OutContainer, AK_REQUIRES(AkTraits::isAppendable<OutContainer>)>
OutContainer copyContainer(const RangeLike &range)
{
    OutContainer rv;
    rv.reserve(range.size());
    for (auto &&v : range) {
        rv.push_back(std::move(v));
    }
    return rv;
}

template<typename RangeLike, typename OutContainer, AK_REQUIRES(AkTraits::isInsertable<OutContainer>)>
OutContainer copyContainer(const RangeLike &range)
{
    OutContainer rv;
    rv.reserve(range.size());
    for (const auto &v : range) {
        rv.insert(v); // Qt containers lack move-enabled insert() overload
    }
    return rv;
}

template<typename RangeList, typename OutContainer>
OutContainer copyAssocContainer(const RangeList &range)
{
    OutContainer rv;
    for (const auto &v : range) {
        rv.insert(v.first, v.second); // Qt containers lack move-enabled insert() overload
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

// Without QT_STRICT_ITERATORS QList and QList iterators do not satisfy STL
// iterator concepts since they are nothing more but typedefs to T* - for those
// we need to provide custom traits.
template<typename Iterator>
struct IteratorTrait<Iterator *> {
    // QTypedArrayData::iterator::iterator_category
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Iterator;
    using difference_type = qsizetype;
    using pointer = Iterator *;
    using reference = Iterator &;
};

template<typename Iterator>
struct IteratorTrait<const Iterator *> {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Iterator;
    using difference_type = qsizetype;
    using pointer = const Iterator *;
    using reference = const Iterator &;
};

template<typename IterImpl, typename RangeLike, typename Iterator = typename RangeLike::const_iterator>
struct IteratorBase {
public:
    using iterator_category = typename IteratorTrait<Iterator>::iterator_category;
    using value_type = typename IteratorTrait<Iterator>::value_type;
    using difference_type = typename IteratorTrait<Iterator>::difference_type;
    using pointer = typename IteratorTrait<Iterator>::pointer;
    using reference = typename IteratorTrait<Iterator>::reference;

    IteratorBase(const IteratorBase<IterImpl, RangeLike> &other)
        : mIter(other.mIter)
        , mRange(other.mRange)
    {
    }

    IterImpl &operator++()
    {
        ++static_cast<IterImpl *>(this)->mIter;
        return *static_cast<IterImpl *>(this);
    }

    IterImpl operator++(int)
    {
        auto ret = *static_cast<IterImpl *>(this);
        ++static_cast<IterImpl *>(this)->mIter;
        return ret;
    }

    bool operator==(const IterImpl &other) const
    {
        return mIter == other.mIter;
    }

    bool operator!=(const IterImpl &other) const
    {
        return !(*static_cast<const IterImpl *>(this) == other);
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
        : mIter(iter)
        , mRange(range)
    {
    }
    IteratorBase(const Iterator &iter, RangeLike &&range)
        : mIter(iter)
        , mRange(std::move(range))
    {
    }

    Iterator mIter;
    RangeLike mRange;
};

template<typename RangeLike, typename TransformFn, typename Iterator = typename RangeLike::const_iterator>
struct TransformIterator : public IteratorBase<TransformIterator<RangeLike, TransformFn>, RangeLike> {
private:
    template<typename... T>
    struct ResultOf;

    template<typename R, typename... Args>
    struct ResultOf<R(Args...)> {
        using type = R;
    };

    template<typename... Ts>
    using FuncHelper = decltype(std::invoke(std::declval<Ts>()...))(Ts...);
    using IteratorValueType = typename ResultOf<FuncHelper<TransformFn, typename IteratorTrait<Iterator>::value_type>>::type;

public:
    using value_type = IteratorValueType;
    using pointer = IteratorValueType *; // FIXME: preserve const-ness
    using reference = const IteratorValueType &; // FIXME: preserve const-ness

    TransformIterator(const Iterator &iter, const TransformFn &fn, const RangeLike &range)
        : IteratorBase<TransformIterator<RangeLike, TransformFn>, RangeLike>(iter, range)
        , mFn(fn)
    {
    }

    auto operator*() const
    {
        return std::invoke(mFn, *this->mIter);
    }

private:
    TransformFn mFn;
};

template<typename RangeLike, typename Predicate, typename Iterator = typename RangeLike::const_iterator>
class FilterIterator : public IteratorBase<FilterIterator<RangeLike, Predicate>, RangeLike>
{
public:
    // Filter iterator is just an InputIterator (for complexity reasons).
    // It actually makes it more efficient with STL algos like std::find()
    using iterator_category = std::input_iterator_tag;

    FilterIterator(const Iterator &iter, const Iterator &end, const Predicate &predicate, const RangeLike &range)
        : IteratorBase<FilterIterator, RangeLike>(iter, range)
        , mPredicate(predicate)
        , mEnd(end)
    {
        while (this->mIter != mEnd && !std::invoke(mPredicate, *this->mIter)) {
            ++this->mIter;
        }
    }

    auto &operator++()
    {
        if (this->mIter != mEnd) {
            do {
                ++this->mIter;
            } while (this->mIter != mEnd && !std::invoke(mPredicate, *this->mIter));
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

template<typename RangeLike, typename Iterator = typename RangeLike::const_iterator>
class EnumerateIterator : public IteratorBase<EnumerateIterator<RangeLike>, RangeLike>
{
public:
    using value_type = std::pair<qsizetype, typename Iterator::value_type>;
    using pointer = value_type *; // FIXME: preserve const-ness
    using reference = const value_type &; // FIXME: preserve const-ness

    EnumerateIterator(const Iterator &iter, qsizetype start, const RangeLike &range)
        : IteratorBase<EnumerateIterator, RangeLike>(iter, range)
        , mCount(start)
    {
    }

    auto &operator++()
    {
        ++mCount;
        ++this->mIter;
        return *this;
    }

    auto &operator++(int)
    {
        auto it = *this;
        ++(*this);
        return it;
    }

    QPair<qsizetype, typename Iterator::value_type> operator*() const
    {
        return qMakePair(mCount, *this->mIter);
    }

private:
    qsizetype mCount = 0;
};

template<typename Container, int Pos, typename Iterator = typename Container::const_key_value_iterator>
class AssociativeContainerIterator : public IteratorBase<AssociativeContainerIterator<Container, Pos>, Container, Iterator>
{
public:
    using value_type = std::remove_const_t<std::remove_reference_t<typename std::tuple_element<Pos, typename Iterator::value_type>::type>>;
    using pointer = std::add_pointer_t<value_type>;
    using reference = std::add_lvalue_reference_t<value_type>;

    AssociativeContainerIterator(const Iterator &iter, const Container &container)
        : IteratorBase<AssociativeContainerIterator<Container, Pos>, Container, Iterator>(iter, container)
    {
    }

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
struct Range {
public:
    using iterator = Iterator;
    using const_iterator = Iterator;
    using value_type = typename detail::IteratorTrait<Iterator>::value_type;

    Range(Iterator &&begin, Iterator &&end)
        : mBegin(std::move(begin))
        , mEnd(std::move(end))
    {
    }

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

// Tags

template<template<typename> class Cont>
struct ToTag_ {
    template<typename T>
    using OutputContainer = Cont<T>;
};

template<template<typename, typename> class Cont>
struct ToAssocTag_ {
    template<typename Key, typename Value>
    using OuputContainer = Cont<Key, Value>;
};

struct ValuesTag_ {
};
struct KeysTag_ {
};

template<typename UnaryOperation>
struct TransformTag_ {
    UnaryOperation mFn;
};

template<typename UnaryPredicate>
struct FilterTag_ {
    UnaryPredicate mFn;
};

template<typename UnaryOperation>
struct ForEachTag_ {
    UnaryOperation mFn;
};

template<typename UnaryPredicate>
struct AllTag_ {
    UnaryPredicate mFn;
};

template<typename UnaryPredicate>
struct AnyTag_ {
    UnaryPredicate mFn;
};

template<typename UnaryPredicate>
struct NoneTag_ {
    UnaryPredicate mFn;
};

struct EnumerateTag_ {
    qsizetype mStart = 0;
};

} // namespace detail
} // namespace AkRanges

// Generic operator| for To_<> converter
template<typename RangeLike, template<typename> class OutContainer, typename T = typename RangeLike::value_type>
auto operator|(const RangeLike &range, AkRanges::detail::ToTag_<OutContainer>) -> OutContainer<T>
{
    using namespace AkRanges::detail;
    return copyContainer<RangeLike, OutContainer<T>>(range);
}

// Specialization for case when InContainer and OutContainer are identical
// Create a copy, but for Qt container this is very cheap due to implicit sharing.
template<template<typename> class InContainer, typename T>
auto operator|(const InContainer<T> &in, AkRanges::detail::ToTag_<InContainer>) -> InContainer<T>
{
    return in;
}

// Generic operator| for ToAssoc_<> converter
template<typename RangeLike, template<typename, typename> class OutContainer, typename T = typename RangeLike::value_type>
auto operator|(const RangeLike &range, AkRanges::detail::ToAssocTag_<OutContainer>) -> OutContainer<typename T::first_type, typename T::second_type>
{
    using namespace AkRanges::detail;
    return copyAssocContainer<RangeLike, OutContainer<typename T::first_type, typename T::second_type>>(range);
}

// Generic operator| for transform()
template<typename RangeLike, typename UnaryOperation>
auto operator|(const RangeLike &range, AkRanges::detail::TransformTag_<UnaryOperation> t)
{
    using namespace AkRanges::detail;
    using OutIt = TransformIterator<RangeLike, UnaryOperation>;
    return Range<OutIt>(OutIt(std::cbegin(range), t.mFn, range), OutIt(std::cend(range), t.mFn, range));
}

// Generic operator| for filter()
template<typename RangeLike, typename UnaryPredicate>
auto operator|(const RangeLike &range, AkRanges::detail::FilterTag_<UnaryPredicate> p)
{
    using namespace AkRanges::detail;
    using OutIt = FilterIterator<RangeLike, UnaryPredicate>;
    return Range<OutIt>(OutIt(std::cbegin(range), std::cend(range), p.mFn, range), OutIt(std::cend(range), std::cend(range), p.mFn, range));
}

// Generator operator| for enumerate()
template<typename RangeLike>
auto operator|(const RangeLike &range, AkRanges::detail::EnumerateTag_ tag)
{
    using namespace AkRanges::detail;
    using OutIt = EnumerateIterator<RangeLike>;
    return Range<OutIt>(OutIt(std::cbegin(range), tag.mStart, range), OutIt(std::cend(range), tag.mStart, range));
}

// Generic operator| for foreach()
template<typename RangeLike, typename UnaryOperation>
auto operator|(const RangeLike &range, AkRanges::detail::ForEachTag_<UnaryOperation> op)
{
    std::for_each(std::cbegin(range), std::cend(range), [op = std::move(op)](const auto &val) mutable {
        std::invoke(op.mFn, val);
    });
    return range;
}

// Generic operator| for all
template<typename RangeLike, typename UnaryPredicate>
auto operator|(const RangeLike &range, AkRanges::detail::AllTag_<UnaryPredicate> p)
{
    return std::all_of(std::cbegin(range), std::cend(range), p.mFn);
}

// Generic operator| for any
template<typename RangeLike, typename PredicateFn>
auto operator|(const RangeLike &range, AkRanges::detail::AnyTag_<PredicateFn> p)
{
    return std::any_of(std::cbegin(range), std::cend(range), p.mFn);
}

// Generic operator| for none
template<typename RangeLike, typename UnaryPredicate>
auto operator|(const RangeLike &range, AkRanges::detail::NoneTag_<UnaryPredicate> p)
{
    return std::none_of(std::cbegin(range), std::cend(range), p.mFn);
}

// Generic operator| for keys
template<typename AssocContainer>
auto operator|(const AssocContainer &in, AkRanges::detail::KeysTag_)
{
    using namespace AkRanges::detail;
    using OutIt = AssociativeContainerKeyIterator<AssocContainer>;
    return Range<OutIt>(OutIt(in.constKeyValueBegin(), in), OutIt(in.constKeyValueEnd(), in));
}

// Generic operator| for values
template<typename AssocContainer>
auto operator|(const AssocContainer &in, AkRanges::detail::ValuesTag_)
{
    using namespace AkRanges::detail;
    using OutIt = AssociativeContainerValueIterator<AssocContainer>;
    return Range<OutIt>(OutIt(in.constKeyValueBegin(), in), OutIt(in.constKeyValueEnd(), in));
}

namespace AkRanges
{
namespace Actions
{
/// Non-lazily convert given range or container to QList
static constexpr auto toQVector = detail::ToTag_<QList>{};
/// Non-lazily convert given range or container to QSet
static constexpr auto toQSet = detail::ToTag_<QSet>{};
/// Non-lazily convert given range or container to QList
static constexpr auto toQList = detail::ToTag_<QList>{};
/// Non-lazily convert given range or container of pairs to QMap
static constexpr auto toQMap = detail::ToAssocTag_<QMap>{};
/// Non-lazily convert given range or container of pairs to QHash
static constexpr auto toQHash = detail::ToAssocTag_<QHash>{};

/// Non-lazily call UnaryOperation for each element of the container or range
template<typename UnaryOperation>
detail::ForEachTag_<UnaryOperation> forEach(UnaryOperation &&op)
{
    return detail::ForEachTag_<UnaryOperation>{std::forward<UnaryOperation>(op)};
}

/// Non-lazily check that all elements in the range satisfy given predicate
template<typename UnaryPredicate>
detail::AllTag_<UnaryPredicate> all(UnaryPredicate &&pred)
{
    return detail::AllTag_<UnaryPredicate>{std::forward<UnaryPredicate>(pred)};
}

/// Non-lazily check that at least one element in range satisfies the given predicate
template<typename UnaryPredicate>
detail::AnyTag_<UnaryPredicate> any(UnaryPredicate &&pred)
{
    return detail::AnyTag_<UnaryPredicate>{std::forward<UnaryPredicate>(pred)};
}

/// Non-lazily check that none of the elements in the range satisfies the given predicate
template<typename UnaryPredicate>
detail::NoneTag_<UnaryPredicate> none(UnaryPredicate &&pred)
{
    return detail::NoneTag_<UnaryPredicate>{std::forward<UnaryPredicate>(pred)};
}

} // namespace Actions

namespace Views
{
/// Lazily extract values from an associative container
static constexpr auto values = detail::ValuesTag_{};
/// Lazily extract keys from an associative container
static constexpr auto keys = detail::KeysTag_{};

/// Lazily transform each element of a range or container using given transformation
template<typename UnaryOperation>
detail::TransformTag_<UnaryOperation> transform(UnaryOperation &&op)
{
    return detail::TransformTag_<UnaryOperation>{std::forward<UnaryOperation>(op)};
}

/// Lazily filters a range or container by applying given predicate on each element
template<typename UnaryPredicate>
detail::FilterTag_<UnaryPredicate> filter(UnaryPredicate &&pred)
{
    return detail::FilterTag_<UnaryPredicate>{std::forward<UnaryPredicate>(pred)};
}

/// Lazily enumerate elements in input range
inline detail::EnumerateTag_ enumerate(qsizetype start = 0)
{
    return detail::EnumerateTag_{start};
}

/// Create a range, a view on a container from the given pair fo iterators
template<typename Iterator1, typename Iterator2, typename It = std::remove_reference_t<Iterator1>>
detail::Range<It> range(Iterator1 begin, Iterator2 end)
{
    return detail::Range<It>(std::move(begin), std::move(end));
}

} // namespace Views

} // namespace AkRanges
