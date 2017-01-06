/***************************************************************************
 *   Copyright (C) 2006 by Andreas Gungl <a.gungl@gmx.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef ENTITY_H
#define ENTITY_H

#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QStringList>

class QVariant;
class QSqlDatabase;

namespace Akonadi
{
namespace Server
{

/**
  Base class for classes representing database records. It also contains
  low-level data access and manipulation template methods.
*/
class Entity
{
public:
    typedef qint64 Id;

protected:
    qint64 id() const;
    void setId(qint64 id);

    bool isValid() const;

public:
    template <typename T> static QString joinByName(const QVector<T> &list, const QString &sep)
    {
        QStringList tmp;
        Q_FOREACH (const T &t, list) {
            tmp << t.name();
        }
        return tmp.join(sep);
    }

    /**
      Returns the number of records having @p value in @p column.
      @param column The name of the key column.
      @param value The value used to identify the record.
    */
    template <typename T> inline static int count(const QString &column, const QVariant &value)
    {
        return Entity::countImpl(T::tableName(), column, value);
    }

    /**
      Deletes all records having @p value in @p column.
    */
    template <typename T> inline static bool remove(const QString &column, const QVariant &value)
    {
        return Entity::removeImpl(T::tableName(), column, value);
    }

    /**
      Checks whether an entry in a n:m relation table exists.
      @param leftId Identifier of the left part of the relation.
      @param rightId Identifier of the right part of the relation.
     */
    template <typename T> inline static bool relatesTo(qint64 leftId, qint64 rightId)
    {
        return Entity::relatesToImpl(T::tableName(), T::leftColumn(), T::rightColumn(), leftId, rightId);
    }

    /**
      Adds an entry to a n:m relation table (specified by the template parameter).
      @param leftId Identifier of the left part of the relation.
      @param rightId Identifier of the right part of the relation.
    */
    template <typename T> inline static bool addToRelation(qint64 leftId, qint64 rightId)
    {
        return Entity::addToRelationImpl(T::tableName(), T::leftColumn(), T::rightColumn(), leftId, rightId);
    }

    /**
      Removes an entry from a n:m relation table (specified by the template parameter).
      @param leftId Identifier of the left part of the relation.
      @param rightId Identifier of the right part of the relation.
    */
    template <typename T> inline static bool removeFromRelation(qint64 leftId, qint64 rightId)
    {
        return Entity::removeFromRelationImpl(T::tableName(), T::leftColumn(), T::rightColumn(), leftId, rightId);
    }

    enum RelationSide {
        Left,
        Right
    };

    /**
      Clears all entries from a n:m relation table (specified by the given template parameter).
      @param id Identifier on the relation side.
      @param side The side of the relation.
    */
    template <typename T> inline static bool clearRelation(qint64 id, RelationSide side = Left)
    {
        return Entity::clearRelationImpl(T::tableName(), T::leftColumn(), T::rightColumn(), id, side);
    }

protected:
    Entity();
    explicit Entity(qint64 id);
    ~Entity();

private:
    static int countImpl(const QString &tableName, const QString &column, const QVariant &value);
    static bool removeImpl(const QString &tableName, const QString &column, const QVariant &value);
    static bool relatesToImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId);
    static bool addToRelationImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId);
    static bool removeFromRelationImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 leftId, qint64 rightId);
    static bool clearRelationImpl(const QString &tableName, const QString &leftColumn, const QString &rightColumn, qint64 id, RelationSide side);

private:
    static QSqlDatabase database();
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
  \end
*/
template <template <typename U> class Op>
struct ById {
    typedef bool result_type;
    bool operator()(Entity::Id lhs, Entity::Id rhs) const
    {
        return Op<Entity::Id>()(lhs, rhs);
    }
    template <typename E>
    bool operator()(const E &lhs, const E &rhs) const
    {
        return this->operator()(lhs.id(), rhs.id());
    }
    template <typename E>
    bool operator()(const E &lhs, Entity::Id rhs) const
    {
        return this->operator()(lhs.id(), rhs);
    }
    template <typename E>
    bool operator()(Entity::Id lhs, const E &rhs) const
    {
        return this->operator()(lhs, rhs.id());
    }
};
}

} // namespace Server
} // namespace Akonadi

#endif
