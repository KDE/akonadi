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
    explicit PartStreamer(Connection *connection, ImapStreamParser *parser,
                          const PimItem &pimItem, QObject *parent = 0);
    ~PartStreamer();

    bool stream(const QByteArray &command, bool checkExists, QByteArray &partName,
                qint64 &partSize, bool *changed = 0);

    QByteArray error() const;

Q_SIGNALS:
    void responseAvailable(const Akonadi::Server::Response &response);

private:
    bool streamNonliteral(Part &part, qint64 &partSize, QByteArray &value);
    bool streamLiteral(Part &part, qint64 &partSize, QByteArray &value);
    bool streamLiteralToFile(qint64 dataSize, Part &part, QByteArray &value);
    bool streamLiteralToFileDirectly(qint64 dataSize, Part &part);

    Connection *mConnection;
    ImapStreamParser *mStreamParser;
    PimItem mItem;
    bool mCheckChanged;
    bool mDataChanged;
    QByteArray mError;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_PARTSTREAMER_H
