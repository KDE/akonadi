/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_NOTIFICATIONMESSAGEV2_P_H
#define AKONADI_NOTIFICATIONMESSAGEV2_P_H

#include "notificationmessagev2_p.h"

namespace Akonadi
{

class NotificationMessageHelpers
{
  public:
    template<typename T>
    static bool compareWithoutOpAndParts( const T &left, const T &right )
    {
      return left.entities() == right.entities()
          && left.type() == right.type()
          && left.sessionId() == right.sessionId()
          && left.resource() == right.resource()
          && left.destinationResource() == right.destinationResource()
          && left.parentCollection() == right.parentCollection()
          && left.parentDestCollection() == right.parentDestCollection();
    }


    template<typename List, typename Msg>
    static bool appendAndCompress(List &list, const Msg &msg)
    {
        // There are often multiple Collection Modify notifications in the queue,
        // so we optimize for this case. All other cases are just append, as the
        // compression becomes too expensive in large batches
        if (msg.type() == NotificationMessageV2::Collections && msg.operation() == NotificationMessageV2::Modify) {
            typename List::Iterator iter, begin;
            // We are iterating from end, since there's higher probability of finding
            // matching notification
            for (iter = list.end(), begin = list.begin(); iter != begin; ) {
                --iter;
                if (compareWithoutOpAndParts(msg, (*iter))) {
                    // both are modifications, merge them together and drop the new one
                    if (msg.operation() == NotificationMessageV2::Modify && iter->operation() == NotificationMessageV2::Modify) {
                        iter->setItemParts(iter->itemParts() + msg.itemParts());
                        return false;
                    }

                    // we found Add notification, which means there will be no other
                    // notifications for this entity in the list, so break here
                    if (iter->operation() == NotificationMessageV2::Add) {
                        break;
                    }
                }
            }

        }

        list.append(msg);
        return true;
    }
};

}

#endif
