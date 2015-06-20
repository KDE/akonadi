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

namespace Akonadi {

namespace Protocol {
class PartMetaData;
class Command;
}

namespace Server {

class PimItem;
class Part;
class Connection;
class ImapStreamParser;
class Response;

class PartStreamer : public  QObject
{
    Q_OBJECT

public:
    explicit PartStreamer(Connection *connection, const PimItem &pimItem, QObject *parent = 0);
    ~PartStreamer();

    bool stream(bool checkExists, const QByteArray &partName, qint64 &partSize, bool *changed = 0);
    bool streamAttribute(bool checkExists, const QByteArray &partName, const QByteArray &value, bool *changed = 0);

    QString error() const;

Q_SIGNALS:
    void responseAvailable(const Protocol::Command &response);

private:
    bool streamPayload(Part &part, const QByteArray &partName);
    bool streamPayloadToFile(Part &part, const Protocol::PartMetaData &metaPart);
    bool streamPayloadData(Part &part, const Protocol::PartMetaData &metaPart);

    Protocol::PartMetaData requestPartMetaData(const QByteArray &partName);
    bool preparePart(bool checkExists, const QByteArray &partName, Part &part);

    Connection *mConnection;
    ImapStreamParser *mStreamParser;
    PimItem mItem;
    bool mCheckChanged;
    bool mDataChanged;
    QString mError;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_PARTSTREAMER_H
