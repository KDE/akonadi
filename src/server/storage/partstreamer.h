/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

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
