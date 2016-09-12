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

#include <QObject>

#include "entities.h"

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

class PimItem;
class Part;
class Connection;

class PartStreamer : public  QObject
{
    Q_OBJECT

public:
    explicit PartStreamer(Connection *connection, const PimItem &pimItem, QObject *parent = nullptr);
    ~PartStreamer();

    bool stream(bool checkExists, const QByteArray &partName, qint64 &partSize, bool *changed = nullptr);
    bool streamAttribute(bool checkExists, const QByteArray &partName, const QByteArray &value, bool *changed = nullptr);

    QString error() const;

Q_SIGNALS:
    void responseAvailable(const Protocol::CommandPtr &response);

private:
    bool streamPayload(Part &part, const QByteArray &partName);
    bool streamPayloadToFile(Part &part, const Protocol::PartMetaData &metaPart);
    bool streamPayloadData(Part &part, const Protocol::PartMetaData &metaPart);
    bool streamForeignPayload(Part &part, const Protocol::PartMetaData &metaPart);

    Protocol::PartMetaData requestPartMetaData(const QByteArray &partName);
    bool preparePart(bool checkExists, const QByteArray &partName, Part &part);

    Connection *mConnection;
    PimItem mItem;
    bool mCheckChanged;
    bool mDataChanged;
    QString mError;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_PARTSTREAMER_H
