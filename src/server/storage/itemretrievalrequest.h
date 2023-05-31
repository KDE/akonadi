/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QString>

#include <optional>

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
        explicit Id(uint32_t value)
            : mValue(value)
        {
        }
        bool operator==(Id other) const
        {
            return mValue == other.mValue;
        }

    private:
        uint32_t mValue;
        Id next()
        {
            return Id{++mValue};
        }

        friend class ItemRetrievalRequest;
        friend QDebug operator<<(QDebug, Id);
    };

    explicit ItemRetrievalRequest();

    Id id;
    QList<qint64> ids;
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
    {
    }

    ItemRetrievalRequest request;

    std::optional<QString> errorMsg{};
};

inline QDebug operator<<(QDebug dbg, ItemRetrievalRequest::Id id)
{
    dbg.nospace() << id.mValue;
    return dbg.space();
}

} // namespace Server
} // namespace Akonadi
