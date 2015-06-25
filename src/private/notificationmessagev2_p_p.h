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

class NotificationMessageV2::Private : public QSharedData
{
public:
    Private()
        : QSharedData()
        , type(InvalidType)
        , operation(InvalidOp)
        , parentCollection(-1)
        , parentDestCollection(-1)
    {
    }

    Private(const Private &other)
        : QSharedData(other)
    {
        sessionId = other.sessionId;
        type = other.type;
        operation = other.operation;
        items = other.items;
        resource = other.resource;
        destResource = other.destResource;
        parentCollection = other.parentCollection;
        parentDestCollection = other.parentDestCollection;
        parts = other.parts;
        addedFlags = other.addedFlags;
        removedFlags = other.removedFlags;
        addedTags = other.addedTags;
        removedTags = other.removedTags;
    }

    QByteArray sessionId;
    NotificationMessageV2::Type type;
    NotificationMessageV2::Operation operation;
    QMap<Id, NotificationMessageV2::Entity> items;
    QByteArray resource;
    QByteArray destResource;
    Id parentCollection;
    Id parentDestCollection;
    QSet<QByteArray> parts;
    QSet<QByteArray> addedFlags;
    QSet<QByteArray> removedFlags;
    QSet<qint64> addedTags;
    QSet<qint64> removedTags;

    // For internal use only: Akonadi server can add some additional information
    // that might be useful when evaluating the notification for example, but
    // it is never transfered to clients
    QVector<QByteArray> metadata;
};

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
        //It is likely that compressable notifications are within the last few notifications, so avoid searching a list that is potentially huge
        static const int maxCompressionSearchLength = 10;
        int searchCounter = 0;
        // There are often multiple Collection Modify notifications in the queue,
        // so we optimize for this case.
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

                    // we found Add notification, which means we can drop this modification
                    if (iter->operation() == NotificationMessageV2::Add) {
                        return false;
                    }
                }
                searchCounter++;
                if (searchCounter >= maxCompressionSearchLength) {
                    break;
                }
            }
        }

        // All other cases are just append, as the compression becomes too expensive in large batches
        list.append(msg);
        return true;
    }

    template<template<typename> class Container>
    static QByteArray join(const Container<QByteArray> &v, const QByteArray &separator)
    {
        QByteArray rv;
        for (auto iter = v.cbegin(), end = v.cend(); iter != end; ++iter) {
            if (iter != v.cbegin()) {
                rv += separator;
            }
            rv += (*iter);
        }
        return rv;
    }

};

} // namespace Akonadi

#endif
