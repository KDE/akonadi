/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKLOCALSERVER_H
#define AKLOCALSERVER_H

#include <QLocalServer>

namespace Akonadi
{
namespace Server
{

class AkLocalServer : public QLocalServer
{
    Q_OBJECT
public:
    explicit AkLocalServer(QObject *parent = nullptr);

Q_SIGNALS:
    void newConnection(quintptr socketDescriptor);

protected:
    void incomingConnection(quintptr socketDescriptor) override;
};

} // namespace Server
} // namespace Akonadi

#endif
