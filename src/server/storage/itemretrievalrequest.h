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
#include <QList>

namespace Akonadi {
namespace Server {

/// Details of a single item retrieval request
class ItemRetrievalRequest
{
public:
    ItemRetrievalRequest()
        : processed(false)
    {
    }

    QList<qint64> ids;
    QString resourceId;
    QByteArrayList parts; // list instead of vector to simplify client-side handling
    QString errorMsg;
    bool processed;

private:
    Q_DISABLE_COPY(ItemRetrievalRequest)
};

} // namespace Server
} // namespace Akonadi

#endif
