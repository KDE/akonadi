/*
    Copyright (c) 2019 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_SERVER_QUERYITERATOR_H_
#define AKONADI_SERVER_QUERYITERATOR_H_

#include <QSqlQuery>
#include <QVariant>

#include <iterator>

namespace Akonadi
{
namespace Server
{
namespace Qb
{

class QueryResultIterator;
class QueryResult
{
public:
    QueryResult(const QueryResult &) = default;
    QueryResult &operator=(const QueryResult &) = default;
    QueryResult(QueryResult &&) = default;
    QueryResult &operator=(QueryResult &&) = default;
    ~QueryResult() = default;

    bool operator==(const QueryResult &other) const
    {
        // TODO: user should never compare two iterators from different queries
        // but if they do, we should catch it here...
        return mQuery.isValid() == other.mQuery.isValid()
                || mQuery.at() == other.mQuery.at();
    }

    auto at(int index) const
    {
        return mQuery.isValid() ? mQuery.value(index) : QVariant{};
    }

    template<typename T>
    T at(int index) const
    {
        if (!mQuery.isValid()) {
            return T{};
        }

        const auto value = at(index);
        Q_ASSERT(value.canConvert<T>());
        return value.value<T>();
    }

private:
    friend class QueryResultIterator;
    explicit QueryResult(const QSqlQuery &query)
        : mQuery(query)
    {
        Q_ASSERT(mQuery.at() == QSql::BeforeFirstRow);
        mQuery.next(); // move to the first row
    }

    explicit QueryResult()
    {}

    bool next()
    {
        return mQuery.next();
    }

    bool isValid() const
    {
        return mQuery.isValid();
    }

    QSqlQuery mQuery;
};


class Query;
class QueryResultIterator
{
public:
    using value_type = QueryResult;
    using difference_type = int;
    using reference = const QueryResult &;
    using pointer = QueryResult const *;
    using iterator_category = std::input_iterator_tag;

    using End = bool;

    QueryResultIterator(const QueryResultIterator &) = default;
    QueryResultIterator &operator=(const QueryResultIterator &) = default;
    QueryResultIterator(QueryResultIterator &&) = default;
    QueryResultIterator &operator=(QueryResultIterator &&) = default;
    ~QueryResultIterator() = default;

    reference operator*() const
    {
        return mResult;
    }

    pointer operator->() const
    {
        return &mResult;
    }

    QueryResultIterator &operator++()
    {
        mResult.next();
        return *this;
    }

    QueryResultIterator operator++(int num)
    {
        if (!mResult.isValid()) {
            return *this;
        }

        auto prevResult = mResult;
        for (int i = 0; i < num && mResult.isValid(); ++i, mResult.next());

        return QueryResultIterator(std::move(prevResult));
    }

    bool operator==(const QueryResultIterator &other) const
    {
        return mResult == other.mResult;
    }

    inline bool operator!=(const QueryResultIterator &other) const
    {
        return !(*this == other);
    }

private:
    friend class Query;
    explicit QueryResultIterator(const QSqlQuery &query)
        : mResult(query)
    {}

    explicit QueryResultIterator(QueryResult &&result)
        : mResult(std::move(result))
    {}

    explicit QueryResultIterator(End)
    {}

private:
    QueryResult mResult;
};

} // namespace Qb
} // namespace Server
} // namespace Akonadi

#endif
