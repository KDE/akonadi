/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

namespace Akonadi {

class NotificationMessageHelpers
{
public:
    template<typename T>
    static bool compareWithoutOpAndParts(const T &left, const T &right)
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
    static bool appendAndCompressImpl(List &list, const Msg &msg)
    {
        // fast-path for stuff that is not considered during O(n) compression below
        if (msg.operation() != NotificationMessageV2::Add
            && msg.operation() != NotificationMessageV2::Link
            && msg.operation() != NotificationMessageV2::Unlink
            && msg.operation() != NotificationMessageV2::Subscribe
            && msg.operation() != NotificationMessageV2::Unsubscribe
            && msg.operation() != NotificationMessageV2::Move) {

            typename List::Iterator end = list.end();
            for (typename List::Iterator it = list.begin(); it != end;) {
                if (compareWithoutOpAndParts(msg, (*it))) {

                    // both are modifications, merge them together and drop the new one
                    if (msg.operation() == NotificationMessageV2::Modify && it->operation() == NotificationMessageV2::Modify) {
                        (*it).setItemParts((*it).itemParts() + msg.itemParts());
                        return false;
                    } else if (msg.operation() == NotificationMessageV2::ModifyFlags
                               && it->operation() == NotificationMessageV2::ModifyFlags) {
                        (*it).setAddedFlags((*it).addedFlags() + msg.addedFlags());
                        (*it).setRemovedFlags((*it).removedFlags() + msg.removedFlags());

                        // If merged notifications result in no-change notification, drop both.
                        if ((*it).addedFlags() == (*it).removedFlags()) {
                            it = list.erase(it);
                            end = list.end();
                        }

                        return false;
                    } else if (msg.operation() == NotificationMessageV2::ModifyTags
                               && it->operation() == NotificationMessageV2::ModifyTags) {
                        (*it).setAddedTags((*it).addedTags() + msg.addedTags());
                        (*it).setRemovedTags((*it).removedTags() + msg.removedTags());

                        // If merged notification results in no-change notification, drop both
                        if ((*it).addedTags() == (*it).removedTags()) {
                            it = list.erase(it);
                            end = list.end();
                        }

                        return false;
                    } else if (((msg.operation() == NotificationMessageV2::Modify)
                                || (msg.operation() == NotificationMessageV2::ModifyFlags))
                               && ((*it).operation() != NotificationMessageV2::Modify)
                               && (*it).operation() != NotificationMessageV2::ModifyFlags
                               && (*it).operation() != NotificationMessageV2::ModifyTags) {
                        // new one is a modification, the existing one not, so drop the new one
                        return false;
                    } else if (msg.operation() == NotificationMessageV2::Remove
                               && ((*it).operation() == NotificationMessageV2::Modify
                                   || (*it).operation() == NotificationMessageV2::ModifyFlags
                                   || (*it).operation() == NotificationMessageV2::ModifyTags)) {
                        // new one is a deletion, erase the existing modification ones (and keep going, in case there are more)
                        it = list.erase(it);
                        end = list.end();
                    } else {
                        // keep looking
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }

        list.append(msg);
        return true;
    }
};

}

#endif
