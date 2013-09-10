/*
   Copyright (C) 2013 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef TODOPURGER_H
#define TODOPURGER_H

#include "calendarbase.h"

#include <QObject>

namespace Akonadi {

class IncidenceChanger;

/**
* @short Class to delete completed to-dos.
*
* @author Sérgio Martins <iamsergio@gmail.com>
* @since 4.12
*/
class TodoPurger : public QObject
{
    Q_OBJECT
public:
    explicit TodoPurger(QObject *parent = 0);
    ~TodoPurger();

    /**
     * Sets an IncidenceChanger.
     * If you don't call this method, an internal IncidenceChanger will be created.
     * Use this if you want more control over the deletion operations, like iTip management, ACL, undo/redo support.
     */
    void setIncidenceChager(IncidenceChanger *changer);

    /**
     * Sets the calendar to be used for retrieving the to-do hierarchy.
     * If you don't call this method, an internal FetchJobCalendar will be created.
     * Use this if you want to reuse an existing calendar, for performance reasons for example.
     */
    void setCalendar(const CalendarBase::Ptr &calendar);

    /**
     * Deletes completed to-dos. A to-do with uncomplete children won't be deleted.
     * @see purgeCompletedTodos()
     */
    void purgeCompletedTodos();

    /**
     * Use this after receiving the an unsuccessful todosPurged() signal to get a i18n error message.
     */
    QString lastError() const;

Q_SIGNALS:
    /**
     * Emitted when purging completed to-dos finished.
     * @param success    True if the operation could be completed. @see lastError()
     * @param numDeleted Number of to-dos that were deleted.
     * @param numIgnored Number of to-dos that weren't deleted because they have uncomplete childs.
     */
    void todosPurged(bool success, int numDeleted, int numIgnored);

private:
    class Private;
    Private *const d;
};

}

#endif
