/*
  Copyright (c) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    Author: Tobias Koenig <tokoe@kdab.com>

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

#ifndef AKONADI_BLOCKALARMSATTRIBUTE_H
#define AKONADI_BLOCKALARMSATTRIBUTE_H

#include "akonadi-calendar_export.h"
#include <kcalcore/alarm.h>

#include <akonadi/attribute.h>

namespace Akonadi {

/**
 * @short An Attribute that marks that alarms from a calendar collection are blocked.
 *
 * A calendar collection which has this attribute set won't be evaluated by korgac and
 * therefore it's alarms won't be used, unless explicitly unblocked in blockAlarmType().
 *
 * @author Tobias Koenig <tokoe@kdab.com>
 * @see Akonadi::Attribute
 * @since 4.11
 */
class AKONADI_CALENDAR_EXPORT BlockAlarmsAttribute : public Akonadi::Attribute
{
public:
    /**
      * Creates a new block alarms attribute.
      */
    BlockAlarmsAttribute();

    /**
      * Destroys the block alarms attribute.
      */
    ~BlockAlarmsAttribute();

    /**
     * Blocks or unblocks given alarm type.
     *
     * By default, all alarm types are blocked.
     *
     * @since 4.11
     */
    void blockAlarmType(KCalCore::Alarm::Type type, bool block = true);

    /**
     * Blocks or unblocks every alarm type.
     *
     * By default, all alarm types are blocked.
     *
     * @since 5.0
     */
    void blockEverything(bool block = true);

    /**
     * Returns whether given alarm type is blocked or not.
     *
     * @since 4.11
     */
    bool isAlarmTypeBlocked(KCalCore::Alarm::Type type) const;

    /**
     * Returns whether all alarms are blocked or not.
     *
     * @since 5.0
     */

    bool isEverythingBlocked() const;

    /**
      * Reimplemented from Attribute
      */
    QByteArray type() const;

    /**
      * Reimplemented from Attribute
      */
    BlockAlarmsAttribute *clone() const;

    /**
      * Reimplemented from Attribute
      */
    QByteArray serialized() const;

    /**
      * Reimplemented from Attribute
      */
    void deserialize(const QByteArray &data);

private:
    class Private;
    Private * const d;
};

}

#endif
