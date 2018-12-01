/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_SERVER_PARTSTREAMER_H
#define AKONADI_SERVER_PARTSTREAMER_H

#include <QSharedPointer>

#include "entities.h"
#include "exception.h"

namespace Akonadi
{

namespace Protocol
{
class PartMetaData;
class Command;
using CommandPtr = QSharedPointer<Command>;
}

namespace Server
{

AKONADI_EXCEPTION_MAKE_INSTANCE(PartStreamerException);

class PimItem;
class Part;
class Connection;

class PartStreamer
{
public:
    explicit PartStreamer(Connection *connection, const PimItem &pimItem);
    ~PartStreamer();

    /**
     * @throws PartStreamException
     */
    void stream(bool checkExists, const QByteArray &partName, qint64 &partSize, bool *changed = nullptr);

    /**
     * @throws PartStreamerException
     */
    void streamAttribute(bool checkExists, const QByteArray &partName, const QByteArray &value, bool *changed = nullptr);

private:
    void streamPayload(Part &part, const QByteArray &partName);
    void streamPayloadToFile(Part &part, const Protocol::PartMetaData &metaPart);
    void streamPayloadData(Part &part, const Protocol::PartMetaData &metaPart);
    void streamForeignPayload(Part &part, const Protocol::PartMetaData &metaPart);

    Protocol::PartMetaData requestPartMetaData(const QByteArray &partName);
    void preparePart(bool checkExists, const QByteArray &partName, Part &part);

    Connection *mConnection;
    PimItem mItem;
    bool mCheckChanged;
    bool mDataChanged;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_PARTSTREAMER_H
