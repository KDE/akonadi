/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Andreas Gungl <a.gungl@gmx.de>           *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QString>
#include <QStringList>

class QVariant;

namespace Akonadi
{
namespace Server
{
class DataStore;
/**
  Base class for classes representing database records. It also contains
  low-level data access and manipulation template methods.
*/
class Entity
{
public:
    using Id = qint64;

protected:
    qint64 id() const;
    void setId(qint64 id);

    bool isValid() const;

public:
    template<typename T>
    static QString joinByName(const QList<T> &list, const QString &sep)
    {
        QStringList tmp;
        tmp.reserve(list.count());
        for (const T &t : list) {
            tmp << t.name();
        }
        return tmp.join(sep);
    }

    /**
      Returns the number of records having @p value in @p column.
      @param column The name of the key column.
      @param value The value used to identify the record.
    */
    template<typename T>
    inline static int count(const QString &column, const QVariant &value)
    {
        return count<T>(dataStore(), column, value);
    }

    template<typename T>
    inline static int count(DataStore *store, const QString &column, const QVariant &value)
    {
        return Entity::countImpl(store, T::tableName(), column, value);
    }

    /**
      Deletes all records having @p value in @p column.
    */
    template<typename T>
    inline static bool remove(const QString &column, const QVariant &value)
    {
        return remove<T>(dataStore(), column, value);
    }

    template<typename T>
    inline static bool remove(DataStore *store, const QString &column, const QVariant &value)
    {
        return Entity::removeImpl(store, T::tableName(), column, value);
    }

    /**
      Checks whether an entry in a n:m relation table exists.
      @param leftId Identifier of the left part of the relation.
      @param rightId Identifier of the right part of the relation.
     */
    template<typename T>
    inline static bool relatesTo(qint64 leftId, qint64 rightId)
    {
        return relatesTo<T>(dataStore(), leftId, rightId);
    }

    template<typename T>
    inline static bool relatesTo(DataStore *store, qint64 leftId, qint64 rightId)
    {
        return Entity::relatesToImpl(store, T::tableName(), T::leftColumn(), T::rightColumn(), leftId, rightId);
    }

    /**
      Adds an entry to a n:m relation table (specified by the template parameter).
      @param leftId Identifier of the left part of the relation.
      @param rightId Identifier of the right part of the relation.
    */
    template<typename T>
    inline static bool addToRelation(qint64 leftId, qint64 rightId)
    {
        return addToRelation<T>(dataStore(), leftId, rightId);
    }

    template<typename T>
    inline static bool addToRelation(DataStore *store, qint64 leftId, qint64 rightId)
    {
        return Entity::addToRelationImpl(store, T::tableName(), T::leftColumn(), T::rightColumn(), leftId, rightId);
    }

    /**
      Removes an entry from a n:m relation table (specified by the template parameter).
      @param leftId Identifier of the left part of the relation.
      @param rightId Identifier of the right part of the relation.
    */
    template<typename T>
    inline static bool removeFromRelation(qint64 leftId, qint64 rightId)
    {
        return removeFromRelation<T>(dataStore(), leftId, rightId);
    }

    template<typename T>
    inline static bool removeFromRelation(DataStore *store, qint64 leftId, qint64 rightId)
    {
        return Entity::removeFromRelationImpl(store, T::tableName(), T::leftColumn(), T::rightColumn(), leftId, rightId);
    }

    enum RelationSide {
        Left,
        Right,
    };

    /**
      Clears all entries from a n:m relation table (specified by the given template parameter).
      @param id Identifier on the relation side.
      @param side The side of the relation.
    */
    template<typename T>
    inline static bool clearRelation(qint64 id, RelationSide side = Left)
    {
        return clearRelation<T>(dataStore(), id, side);
    }
    template<typename T>
    inline static bool clearRelation(DataStore *store, qint64 id, RelationSide side = Left)
    {
        return Entity::clearRelationImpl(store, T::tableName(), T::leftColumn(), T::rightColumn(), id, side);
    }

protected:
    Entity();
    explicit Entity(qint64 id);
    ~Entity();

private:
    static DataStore *dataStore();

    static int countImpl(DataStore *store, const QString &tableName, const QString &column, const QVariant &value);
    static bool removeImpl(DataStore *store, const QString &tableName, const QString &column, const QVariant &value);
    static bool relatesToImpl(DataStore *store, const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId);
    static bool
    addToRelationImpl(DataStore *store, const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId);
    static bool
    removeFromRelationImpl(DataStore *store, const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId);
    static bool
    clearRelationImpl(DataStore *store, const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 id, RelationSide side);

private:
    qint64 m_id;
};

namespace _detail
{
/*!
  Binary predicate to sort collections of Entity subclasses by
  their id.

  Example for sorting:
  \code
  std::sort( coll.begin(), coll.end(), _detail::ById<std::less>() );
  \endcode

  Example for finding by id:
  \code
  // linear:
  std::find_if( coll.begin(), coll.end(), bind( _detail::ById<std::equal_to>(), _1, myId ) );
  // binary:
  std::lower_bound( coll.begin(), coll.end(), myId, _detail::ById<std::less>() );
  \endcode
*/
template<template<typename U> class Op>
struct ById {
    using result_type = bool;
    bool operator()(Entity::Id lhs, Entity::Id rhs) const
    {
        return Op<Entity::Id>()(lhs, rhs);
    }
    template<typename E>
    bool operator()(const E &lhs, const E &rhs) const
    {
        return this->operator()(lhs.id(), rhs.id());
    }
    template<typename E>
    bool operator()(const E &lhs, Entity::Id rhs) const
    {
        return this->operator()(lhs.id(), rhs);
    }
    template<typename E>
    bool operator()(Entity::Id lhs, const E &rhs) const
    {
        return this->operator()(lhs, rhs.id());
    }
};
}

} // namespace Server
} // namespace Akonadi
