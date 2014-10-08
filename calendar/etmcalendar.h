/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>
   Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

class EntityTreeModel;
class ChangeRecorder;
class ETMCalendarPrivate;
class CollectionSelection;

/**
 * @short A KCalCore::Calendar that uses an EntityTreeModel to populate itself.
 *
 * All non-idempotent KCalCore::Calendar methods interact with Akonadi.
 * If you need a need a non-persistent calendar use FetchJobCalendar.
 *
 * ETMCalendar allows to be populated with only a subset of your calendar items,
 * by using a KCheckableProxyModel to specify which collections to be used
 * and/or by setting a KCalCore::CalFilter.
 *
 * @see FetchJobCalendar
 * @see CalendarBase
 *
 * @author Sérgio Martins <iamsergio@gmail.com>
 * @since 4.11
 */
class AKONADI_CALENDAR_EXPORT ETMCalendar : public CalendarBase
{
    Q_OBJECT
public:

    enum CollectionColumn {
        CollectionTitle = 0,
        CollectionColumnCount
    };

    typedef QSharedPointer<ETMCalendar> Ptr;

    /**
      * Constructs a new ETMCalendar. Loading begins immediately, asynchronously.
      */
    explicit ETMCalendar(QObject *parent = 0);

    /**
     * Constructs a new ETMCalendar that will only load the specified mime types.
     * Use this ctor to ignore journals or to-dos for example.
     * If no mime types are specified, all mime types will be used.
     */
    explicit ETMCalendar(const QStringList &mimeTypes, QObject *parent = 0);

    /**
     * Constructs a new ETMCalendar.
     *
     * This overload exists for optimization reasons, it allows to share an EntityTreeModel across
     * several ETMCalendars to save memory.
     *
     * Usually when having many ETMCalendars, the only bit that's different is the collection
     * selection. The memory hungry EntityTreeModel is the same, so should be shared.
     *
     * @param calendar an existing ETMCalendar who's EntityTreeModel is to be used.
     *
     * @since 4.13
     */
    explicit ETMCalendar(ETMCalendar *calendar, QObject *parent = 0);

    explicit ETMCalendar(ChangeRecorder *monitor, QObject *parent = 0);
    /**
      * Destroys this ETMCalendar.
      */
    ~ETMCalendar();

    /**
     * Returns the collection having @p id.
     * Use this instead of creating a new collection, the returned collection will have
     * the correct right, name, display name, etc all set.
     */
    Akonadi::Collection collection(Akonadi::Collection::Id) const;

    /**
      * Returns true if the collection owning incidence @p has righ @p right
      */
    bool hasRight(const Akonadi::Item &item, Akonadi::Collection::Right right) const;

    /**
      * This is an overloaded function.
      * @param uid the identifier for the incidence to check for rights
      * @param right the access right to check for
      * @see hasRight()
      */
    bool hasRight(const QString &uid, Akonadi::Collection::Right right) const;

    /**
     * Returns the KCheckableProxyModel used to select from which collections should
     * the calendar be populated from.
     */
    KCheckableProxyModel *checkableProxyModel() const;

    /**
     * Convenience method to access the contents of this KCalCore::Calendar through
     * a QAIM interface.
     *
     * Like through calendar interface, the model only items of selected collections.
     * To select or unselect collections, see checkableProxyModel().
     *
     * @see checkableProxyModel()
     * @see entityTreeModel()
     */
    QAbstractItemModel *model() const;

    /**
     * Returns the underlying EntityTreeModel.
     *
     * For most uses, you'll want model() or the KCalCore::Calendar interface instead.
     *
     * It contains every item and collection with calendar mime type, doesn't have
     * KCalCore filtering and doesn't honour any collection selection.
     *
     * This method is exposed for performance reasons only, so you can reuse it,
     * since it's resource savy.
     *
     * @see model()
     */
    Akonadi::EntityTreeModel *entityTreeModel() const;

    /**
      * Returns all alarms occurring in a specified time interval.
      * @param from start date of interval
      * @param to end data of interval
      * @param exludeBlockedAlarms if true, alarms belonging to blocked collections aren't returned.
      * TODO_KDE5: introduce this overload in KCalCore::Calendar, MemoryCalendar, etc. all the way
      * up the hierarchy
      */
    using KCalCore::MemoryCalendar::alarms;
    KCalCore::Alarm::List alarms(const KDateTime &from,
                                 const KDateTime &to,
                                 bool excludeBlockedAlarms) const;

    /**
     * Enable or disable collection filtering.
     * If true, the calendar will only contain items of selected collections.
     * @param enable enables collection filtering if set as @c true
     * @see checkableProxyModel()
     * @see collectionFilteringEnabled()
     */
    void setCollectionFilteringEnabled(bool enable);

    /**
     * Returns whether collection filtering is enabled. Default is true.
     * @see setCollectionFilteringEnabled()
     */
    bool collectionFilteringEnabled() const;

    /**
      * Returns if the calendar already finished loading.
      * TODO_KDE5: make virtual in base class
      */
    bool isLoaded() const;

Q_SIGNALS:
    /**
      * This signal is emitted if a collection has been changed (properties or attributes).
      *
      * @param collection The changed collection.
      * @param attributeNames The names of the collection attributes that have been changed.
      */
    void collectionChanged(const Akonadi::Collection &, const QSet<QByteArray> &attributeNames);

    /**
      * This signal is emitted when one or more collections are added to the ETM.
      *
      * @param collection non empty list of collections
      */
    void collectionsAdded(const Akonadi::Collection::List &collection);

    /**
      * This signal is emitted when one or more collections are deleted from the ETM.
      *
      * @param collection non empty list of collections
      */
    void collectionsRemoved(const Akonadi::Collection::List &collection);

    /**
      * Emitted whenever an Item is inserted, removed or modified.
      */
    void calendarChanged();

private:
    Q_DECLARE_PRIVATE(ETMCalendar)
};
}

#endif
