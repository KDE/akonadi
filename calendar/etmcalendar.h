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
#include "akonadicalendar.h"

#include <Akonadi/Collection>
#include <KSystemTimeZones>

class QAbstractItemModel;
class KCheckableProxyModel;

namespace Akonadi {
  class ETMCalendarPrivate;
  class CollectionSelection;
  class AKONADI_CALENDAR_EXPORT ETMCalendar : public AkonadiCalendar
  {
  Q_OBJECT
  public:
    typedef QSharedPointer<ETMCalendar> Ptr;

    explicit ETMCalendar( const KDateTime::Spec &timeSpec = KSystemTimeZones::local() );

    /**
     * Destroys this ETMCalendar.
     */
    ~ETMCalendar();

    Akonadi::Collection collection( Akonadi::Collection::Id ) const;

    /**
     * Returns true if @p item can be deleted from it's collection.
     */
    bool hasDeleteRights( const Akonadi::Item &item ) const; //TODO

    /**
     * Returns true if the incidence with @p uid can be
     * deleted from it's collection.
     */
    bool hasDeleteRights( const QString &uid ) const;

    /**
     * Returns true if @p item can be modified.
     */    
    bool hasModifyRights( const Akonadi::Item &item ) const;

    /**
     * Returns true if the incidence with @p uid modified.
     */    
    bool hasModifyRights( const QString &uid ) const;

    QAbstractItemModel *unfilteredModel() const;
    QAbstractItemModel *filteredModel() const;
    KCheckableProxyModel *checkableProxyModel() const;
    Akonadi::CollectionSelection *collectionSelection() const;

  Q_SIGNALS:
    /**
     * This signal is emitted if a collection has been changed (properties or attributes).
     *
     * @param collection The changed collection.
     * @param attributeNames The names of the collection attributes that have been changed.
     */
    void collectionChanged( const Akonadi::Collection &, const QSet<QByteArray> &attributeNames );

  private:
    Q_DECLARE_PRIVATE( ETMCalendar );
  };
}

#endif
