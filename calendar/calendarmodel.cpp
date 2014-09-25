/*
  Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>
                2009 KDAB; Author: Frank Osterfeld <osterfeld@kde.org>

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

#include "calendarmodel_p.h"

#include "utils_p.h"

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <kcalcore/event.h>
#include <kcalcore/todo.h>
#include <kcalcore/journal.h>

#include <KLocalizedString>
#include <KIconLoader>

#include <QPixmap>

using namespace Akonadi;

static KCalCore::Todo::Ptr todo(const Akonadi::Item &item)
{
    return
        item.hasPayload<KCalCore::Todo::Ptr>() ?
        item.payload<KCalCore::Todo::Ptr>() :
        KCalCore::Todo::Ptr();
}

class CalendarModel::Private
{
public:
    Private()
    {
    }

    QWeakPointer<CalendarModel> m_weakPointer;
};

CalendarModel::CalendarModel(Akonadi::ChangeRecorder *monitor)
    : EntityTreeModel(monitor),
      d(new Private())
{
    monitor->itemFetchScope().fetchAllAttributes(true);
}

CalendarModel::Ptr CalendarModel::create(ChangeRecorder *monitor)
{
    CalendarModel *model = new CalendarModel(monitor);
    CalendarModel::Ptr modelPtr = CalendarModel::Ptr(model);
    model->setWeakPointer(modelPtr.toWeakRef());
    return modelPtr;
}

CalendarModel::~CalendarModel()
{
    delete d;
}

QWeakPointer<CalendarModel> CalendarModel::weakPointer() const
{
    return d->m_weakPointer;
}

void CalendarModel::setWeakPointer(const QWeakPointer<CalendarModel> &weakPointer)
{
    d->m_weakPointer = weakPointer;
}

QVariant CalendarModel::entityData(const Akonadi::Item &item, int column, int role) const
{
    const KCalCore::Incidence::Ptr inc = CalendarUtils::incidence(item);
    if (!inc) {
        return QVariant();
    }

    switch (role) {
    case Qt::DecorationRole:
        if (column != Summary) {
            return QVariant();
        }
        if (inc->type() == KCalCore::IncidenceBase::TypeTodo) {
            return SmallIcon(QLatin1String("view-pim-tasks"));
        }
        if (inc->type() == KCalCore::IncidenceBase::TypeJournal) {
            return SmallIcon(QLatin1String("view-pim-journal"));
        }
        if (inc->type() == KCalCore::IncidenceBase::TypeEvent) {
            return SmallIcon(QLatin1String("view-calendar"));
        }
        return SmallIcon(QLatin1String("network-wired"));

    case Qt::DisplayRole:
        switch (column) {
        case Summary:
            return inc->summary();

        case DateTimeStart:
            return inc->dtStart().toString();

        case DateTimeEnd:
            return inc->dateTime(KCalCore::Incidence::RoleEndTimeZone).toString();

        case DateTimeDue:
            if (KCalCore::Todo::Ptr t = todo(item)) {
                return t->dtDue().toString();
            } else {
                return QVariant();
            }

        case Priority:
            if (KCalCore::Todo::Ptr t = todo(item)) {
                return t->priority();
            } else {
                return QVariant();
            }

        case PercentComplete:
            if (KCalCore::Todo::Ptr t = todo(item)) {
                return t->percentComplete();
            } else {
                return QVariant();
            }

        case Type:
            return inc->typeStr();
        default:
            break;
        }

    case SortRole:
        switch (column) {
        case Summary:
            return inc->summary();

        case DateTimeStart:
            return inc->dtStart().toUtc().dateTime();

        case DateTimeEnd:
            return inc->dateTime(KCalCore::Incidence::RoleEndTimeZone).toUtc().dateTime();

        case DateTimeDue:
            if (KCalCore::Todo::Ptr t = todo(item)) {
                return t->dtDue().toUtc().dateTime();
            } else {
                return QVariant();
            }

        case Priority:
            if (KCalCore::Todo::Ptr t = todo(item)) {
                return t->priority();
            } else {
                return QVariant();
            }

        case PercentComplete:
            if (KCalCore::Todo::Ptr t = todo(item)) {
                return t->percentComplete();
            } else {
                return QVariant();
            }

        case Type:
            return inc->type();

        default:
            break;
        }

        return QVariant();

    case RecursRole:
        return inc->recurs();

    default:
        return QVariant();
    }

    return QVariant();
}

QVariant CalendarModel::entityData(const Akonadi::Collection &collection,
                                   int column, int role) const
{
    return EntityTreeModel::entityData(collection, column, role);
}

int CalendarModel::entityColumnCount(EntityTreeModel::HeaderGroup headerSet) const
{
    if (headerSet == EntityTreeModel::ItemListHeaders) {
        return ItemColumnCount;
    } else {
        return CollectionColumnCount;
    }
}

QVariant CalendarModel::entityHeaderData(int section, Qt::Orientation orientation,
        int role, EntityTreeModel::HeaderGroup headerSet) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    if (headerSet == EntityTreeModel::ItemListHeaders) {
        switch (section) {
        case Summary:
            return i18nc("@title:column calendar event summary", "Summary");
        case DateTimeStart:
            return i18nc("@title:column calendar event start date and time", "Start Date and Time");
        case DateTimeEnd:
            return i18nc("@title:column calendar event end date and time", "End Date and Time");
        case Type:
            return i18nc("@title:column calendar event type", "Type");
        case DateTimeDue:
            return i18nc("@title:column todo item due date and time", "Due Date and Time");
        case Priority:
            return i18nc("@title:column todo item priority", "Priority");
        case PercentComplete:
            return i18nc("@title:column todo item completion in percent", "Complete");
        default:
            return QVariant();
        }
    }

    if (headerSet == EntityTreeModel::CollectionTreeHeaders) {
        switch (section) {
        case CollectionTitle:
            return i18nc("@title:column calendar title", "Calendar");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

