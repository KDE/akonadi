/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>

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

#ifndef _AKONADI_ETMCALENDAR_H_
#define _AKONADI_ETMCALENDAR_H_

#include "akonadi-calendar_export.h"
#include "calendarbase.h"

#include <akonadi/collection.h>

class QAbstractItemModel;
class KCheckableProxyModel;

namespace Akonadi {
  class ETMCalendarPrivate;
  class CollectionSelection;
  class AKONADI_CALENDAR_EXPORT ETMCalendar : public CalendarBase
  {
  Q_OBJECT
  public:

    enum CollectionColumn {
      CollectionTitle=0,
      CollectionColumnCount
    };

    typedef QSharedPointer<ETMCalendar> Ptr;

    /**
     * Constructs a new ETMCalendar. Loading begins immediately, asynchronously.
     */
    explicit ETMCalendar();

    /**
     * Destroys this ETMCalendar.
     */
    ~ETMCalendar();

    Akonadi::Collection collection( Akonadi::Collection::Id ) const;

    /**
     * Returns true if the collection owning incidence @p has righ @p right
     */
    bool hasRight( const Akonadi::Item &item, Akonadi::Collection::Right right ) const;

    /**
     * This is an overloaded function.
     * @see hasRight()
     */
    bool hasRight( const QString &uid, Akonadi::Collection::Right right ) const;

    QAbstractItemModel *unfilteredModel() const;
    QAbstractItemModel *filteredModel() const;
    KCheckableProxyModel *checkableProxyModel() const;

    /**
     * Returns all alarms occuring in a specified time interval.
     * @param from start date of interval
     * @param to end data of interval
     * @param exludeBlockedAlarms if true, alarms belonging to blocked collections aren't returned.
     * TODO_KDE5: introduce this overload in KCalCore::Calendar, MemoryCalendar, etc. all the way
     * up the hierarchy
     */
    using KCalCore::MemoryCalendar::alarms;
    KCalCore::Alarm::List alarms( const KDateTime &from,
                                  const KDateTime &to,
                                  bool excludeBlockedAlarms ) const;

  Q_SIGNALS:
    /**
     * This signal is emitted if a collection has been changed (properties or attributes).
     *
     * @param collection The changed collection.
     * @param attributeNames The names of the collection attributes that have been changed.
     */
    void collectionChanged( const Akonadi::Collection &, const QSet<QByteArray> &attributeNames );

    /**
     * This signal is emitted when one or more collections are added to the ETM.
     *
     * @param collection non empty list of collections
     */
    void collectionsAdded( const Akonadi::Collection::List &collection );

    /**
     * This signal is emitted when one or more collections are deleted from the ETM.
     *
     * @param collection non empty list of collections
     */
    void collectionsRemoved( const Akonadi::Collection::List &collection );

    /**
     * Emitted whenever an Item is inserted, removed or modified.
     */
    void calendarChanged();

  private:
    Q_DECLARE_PRIVATE( ETMCalendar )
  };
}

#endif
