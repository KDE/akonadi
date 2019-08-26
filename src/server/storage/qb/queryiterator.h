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
    auto at(int index) const
    {
        return mQuery.at(index);
    }

    template<typename T>
    T at(int index) const
    {
        const auto &value = mQuery.at(index);
        Q_ASSERT(value.canConvert<T>());
        return value.value<T>();
    }

private:
    friend class QueryResultIterator;
    QueryResult(const QSqlQuery &query)
        : mQuery(query)
    {}

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

    QueryResultIterator(QueryResultIterator &other) = default;
    QueryResultIterator &operator=(QueryResultIterator &other) = default;

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
        if (mQuery.next()) {
            mResult = QueryResult{mQuery};
        } else {
            mEnd = true;
        }

        return *this;
    }

    QueryResultIterator operator++(int num)
    {
        if (mEnd) {
            return *this;
        }

        const auto prevResult = mResult;
        for (int i = 0; i < num; ++i) {
            if (!mQuery.next()) {
                mEnd = true;
                return prevResult;
            }
        }

        mResult = QueryResult{mQuery};
        return *this;
    }

    bool operator==(QueryResultIterator &other) const
    {
        return mEnd == other.mEnd || mResult == other.mResult;
    }

    inline bool operator!=(QueryResultIterator &other) const
    {
        return !(*this == other);
    }

private:
    friend class Qb::Query;
    explicit QueryResultIterator(QSqlQuery &query);
    explicit QueryResultIterator(End);

private:
    QSqlQuery mQuery;
    QueryResult mResult;
    End mEnd = false;
};

} // namespace Qb
} // namespace Server
} // namespace Akonadi

#endif
