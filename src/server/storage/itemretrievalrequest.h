/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef ITEMRETRIEVALREQUEST_H
#define ITEMRETRIEVALREQUEST_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QDebug>

#include <shared/akoptional.h>

namespace Akonadi
{
namespace Server
{

class ItemRetrievalRequest;

/// Details of a single item retrieval request
class ItemRetrievalRequest
{
public:
    struct Id {
        explicit Id(uint32_t value): mValue(value) {};
        bool operator==(const Id &other) const { return mValue == other.mValue; }
    private:
        uint32_t mValue;
        Id next() { return Id{++mValue}; }

        friend class ItemRetrievalRequest;
        friend QDebug operator<<(QDebug, Id);
    };

    explicit ItemRetrievalRequest();

    Id id;
    QVector<qint64> ids;
    QString resourceId;
    QByteArrayList parts; // list instead of vector to simplify client-side handling

private:
    static Id lastId;
};


class ItemRetrievalResult
{
public:
    explicit ItemRetrievalResult() = default; // don't use, sadly Qt metatype system requires type to be default-constructible
    ItemRetrievalResult(ItemRetrievalRequest request)
        : request(std::move(request))
    {}

    ItemRetrievalRequest request;

    akOptional<QString> errorMsg{};
};

inline QDebug operator<<(QDebug dbg, ItemRetrievalRequest::Id id)
{
    dbg.nospace() << id.mValue;
    return dbg.space();
}

} // namespace Server
} // namespace Akonadi
#endif
